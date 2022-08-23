#include "area_recorder.h"

#include "game_manager.h"
#include "beat_clock.h"
#include "audio.h"

namespace {
void SortEventsByTime(std::vector<BeatTimeEvent>& events) {
    std::sort(events.begin(), events.end(), [](BeatTimeEvent const& e1, BeatTimeEvent const& e2) {
        return e1._beatTime < e2._beatTime;
    });
}
}

bool AreaRecorderComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _g = &g;
    return true;
}

constexpr double kNumBeatsPerMeasure = 4.0;

void AreaRecorderComponent::Update(float dt) {
    double currentBeatTime = _g->_beatClock->GetBeatTime();

    if (!_recording) {
        if (!_recordedEvents.empty()) {
            _recording = true;
            _currentBeatNum = 0;
            // Get the downbeat time of the audio event.
            audio::Event const& firstEvent = _recordedEvents.front();
            double firstEventBeatTime = _g->_beatClock->TickTimeToBeatTime(firstEvent.timeInTicks);
            _currentRecordingMeasureStart = _g->_beatClock->GetLastDownBeatTime(firstEventBeatTime);
        }        
    }

    if (_recording) {        
        // Update current measure
        if (currentBeatTime - _currentRecordingMeasureStart >= kNumBeatsPerMeasure) {
            _currentRecordingMeasureStart = currentBeatTime;
        }

        for (audio::Event const& recorded : _recordedEvents) {
            // Convert the absolute event into a beat time event relative to the start of the measure.
            _sequence.emplace_back();
            BeatTimeEvent& b_e = _sequence.back();
            b_e.FromTickTimeEvent(recorded, *_g->_beatClock);
            b_e.MakeTimeRelativeTo(_currentRecordingMeasureStart);                        
        }
        SortEventsByTime(_sequence);
    }
    _recordedEvents.clear();

    // Now let's talk about PLAYBACK. Just like Sequencer: queues events up to a
    // whole beat ahead of schedule.
    if (_recording && !_sequence.empty()) {        
        double const eventCutoffTime = currentBeatTime + 1.0;
        // Make a note to interpret times between [0,1] as being LATER then us.
        bool crossingNextMeasure = eventCutoffTime - _currentRecordingMeasureStart >= kNumBeatsPerMeasure;
        while (true) {
            assert(_currentPlaybackIndex >= 0);
            assert(_currentPlaybackIndex < _sequence.size());
            BeatTimeEvent const& local_b_e = _sequence[_currentPlaybackIndex];
            double absoluteBeatTime = local_b_e._beatTime + _currentRecordingMeasureStart;
            if (crossingNextMeasure && local_b_e._beatTime <= 1.0) {
                absoluteBeatTime += kNumBeatsPerMeasure;
            }
            if (absoluteBeatTime >= currentBeatTime && absoluteBeatTime <= eventCutoffTime) {
                audio::Event tickTimeEvent = local_b_e._e;
                tickTimeEvent.timeInTicks = _g->_beatClock->BeatTimeToTickTime(absoluteBeatTime);
                _g->_audioContext->AddEvent(tickTimeEvent);
                ++_currentPlaybackIndex;
                if (_currentPlaybackIndex > _sequence.size()) {
                    _currentPlaybackIndex = 0;
                }
            } else {
                break;
            }
        }
    }
}

bool AreaRecorderComponent::DrawImGui() {
    return false;
}

void AreaRecorderComponent::Save(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "aabb", _localAabb);
}

void AreaRecorderComponent::Load(serial::Ptree pt) {
    serial::Ptree aabbPt = pt.GetChild("aabb");
    _localAabb.Load(aabbPt);
}

void AreaRecorderComponent::Record(audio::Event const& e) {
    _recordedEvents.push_back(e);
    printf("RECORDED %lu EVENTS\n", _recordedEvents.size());
}