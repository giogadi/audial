#include "sequencer.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "audio.h"

namespace {

std::array<char,1024> gAudioEventScriptBuf;

} // end namespace

void SequencerEntity::Init(GameManager& g) {
    if (g._editMode) {
        _playing = false;
    }
}

void SequencerEntity::Reset() {
    _currentIx = -1;
    _currentLoopStartBeatTime = -1.0;
}

void SequencerEntity::Update(GameManager& g, float dt) {
    if (_events.empty()) {
        return;
    }

    if (!_playing) {
        return;
    }

    // Queue up events up to 1 beat before they should be played.
    double currentBeatTime = g._beatClock->GetBeatTime();
    double maxBeatTime = currentBeatTime + 1.0;
    while (true) {
        if (_currentIx < 0) {
            if (_currentLoopStartBeatTime < 0.0) {
                // This is the first loop iteration. Just start the loop on the next beat.
                _currentLoopStartBeatTime = BeatClock::GetNextBeatDenomTime(currentBeatTime, /*denom=*/1.0);
            }
            _currentIx = 0;
        }
        for (; _currentIx < _events.size(); ++_currentIx) {
            BeatTimeEvent const& b_e = _events[_currentIx];
            double absBeatTime = _currentLoopStartBeatTime + b_e._beatTime;
            if (absBeatTime <= maxBeatTime) {
                // queue it up!
                long tickTime = g._beatClock->BeatTimeToTickTime(absBeatTime);
                audio::Event e = b_e._e;
                e.timeInTicks = tickTime;
                g._audioContext->AddEvent(e);
            } else {
                // not time yet. quit.
                break;
            }
        }
        if (_currentIx < _events.size()) {
            break;
        } else if (_loop) {
            _currentIx = -1;
            double lastEventTime = _events.back()._beatTime;
            assert(lastEventTime > 0.0);  // avoids infinite loop
            _currentLoopStartBeatTime += lastEventTime;
        } else {
            _playing = false;
            break;
        }
    }
}

// TODO: consider using the text format we made above for this.
void SequencerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("loop", _loop);
    serial::Ptree eventsPt = pt.AddChild("events");
    for (BeatTimeEvent const& b_e : _events) {
        serial::Ptree eventPt = eventsPt.AddChild("beat_event");
        b_e.Save(eventPt);
    }
}

void SequencerEntity::LoadDerived(serial::Ptree pt) {
    _loop = pt.GetBool("loop");    
    serial::Ptree eventsPt = pt.TryGetChild("events");
    if (eventsPt.IsValid()) {
        int numChildren = 0;
        serial::NameTreePair* children = eventsPt.GetChildren(&numChildren);
        for (int i = 0; i < numChildren; ++i) {
            _events.emplace_back();
            _events.back().Load(children[i]._pt);
        }
        delete[] children;
    }    
}

ne::Entity::ImGuiResult SequencerEntity::ImGuiDerived(GameManager& g) {
    ImGui::Checkbox("Loop", &_loop);
    if (_playing) {
        if (ImGui::Button("Stop")) {
            Reset();
            _playing = false;
        }
    } else {
        if (ImGui::Button("Play")) {
            Reset();
            _playing = true;
        }
    }
    ImGui::InputTextMultiline("Audio events", gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    if (ImGui::Button("Apply Script to Entity")) {
        ReadBeatEventsFromScript(_events, *g._soundBank, gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    }
    if (ImGui::Button("Apply Entity to Script")) {
        WriteBeatEventsToScript(_events, *g._soundBank, gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    }
    return ImGuiResult::Done;
}