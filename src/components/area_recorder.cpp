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

constexpr double kNumBeatsPerMeasure = 8.0;

void AreaRecorderComponent::Update(float dt) {
    double currentBeatTime = _g->_beatClock->GetBeatTime();

    if (!_recording) {
        if (!_recordedEvents.empty()) {
            _recording = true;
            // Get the downbeat time of the audio event.
            audio::Event const& firstEvent = _recordedEvents.front();
            double firstEventBeatTime = _g->_beatClock->TickTimeToBeatTime(firstEvent.timeInTicks);
            _currentRecordingMeasureStart = _g->_beatClock->GetLastDownBeatTime(firstEventBeatTime);
        }        
    }

    if (_recording) {        
        for (audio::Event const& recorded : _recordedEvents) {
            // Convert the absolute event into a beat time event relative to the start of the measure.
            _sequence.emplace_back();
            BeatTimeEvent& b_e = _sequence.back();
            b_e.FromTickTimeEvent(recorded, *_g->_beatClock);
            b_e._beatTime = std::fmod(b_e._beatTime - _currentRecordingMeasureStart, kNumBeatsPerMeasure);
            assert(b_e._beatTime >= 0.0);             
        }
        SortEventsByTime(_sequence);
    }
    _recordedEvents.clear();

    if (_recording && !_sequence.empty()) {
        double const nextDownBeatTime = _g->_beatClock->GetNextDownBeatTime(currentBeatTime);
        double const curBeatTimeRelToFirstMeasureStart = currentBeatTime - _currentRecordingMeasureStart;
        double const beatTimeOfCurMeasureStart =
            _currentRecordingMeasureStart +
            std::floor(curBeatTimeRelToFirstMeasureStart / kNumBeatsPerMeasure) * kNumBeatsPerMeasure;
        double curBeatTimeInMeasure = std::fmod(curBeatTimeRelToFirstMeasureStart, kNumBeatsPerMeasure);
        double cutoffTimeInMeasure = std::fmod(curBeatTimeInMeasure + 1.0, kNumBeatsPerMeasure);
        bool crossesMeasure = cutoffTimeInMeasure < curBeatTimeInMeasure;
        for (BeatTimeEvent const& b_e : _sequence) {
            double eventAbsBeatTime = -1.0;
            if (crossesMeasure) {
                if (b_e._beatTime <= cutoffTimeInMeasure) {
                    eventAbsBeatTime = nextDownBeatTime + b_e._beatTime;
                } else if (b_e._beatTime >= curBeatTimeInMeasure) {
                    eventAbsBeatTime = beatTimeOfCurMeasureStart + b_e._beatTime;
                }
            } else {
                if (b_e._beatTime >= curBeatTimeInMeasure && b_e._beatTime <= cutoffTimeInMeasure) {
                    eventAbsBeatTime = beatTimeOfCurMeasureStart + b_e._beatTime;
                }
            }
            if (eventAbsBeatTime >= 0.0) {
                audio::Event tickTimeEvent = b_e._e;
                tickTimeEvent.timeInTicks = _g->_beatClock->BeatTimeToTickTime(eventAbsBeatTime);
                _g->_audioContext->AddEvent(tickTimeEvent);
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
}