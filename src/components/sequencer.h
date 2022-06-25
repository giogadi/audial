#pragma once

#include <vector>

#include "component.h"
#include "beat_clock.h"
#include "beat_time_event.h"
#include "audio_event_imgui.h"
#include "audio.h"

class SequencerComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Sequencer; }
    SequencerComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override {
        _audio = g._audioContext;
        _beatClock = g._beatClock;
        return true;
    }

    void SortEventsByTime() {
        std::sort(_events.begin(), _events.end(), [](BeatTimeEvent const& e1, BeatTimeEvent const& e2) {
            return e1._beatTime < e2._beatTime;
        });
    }

    // Keeps event sorted by time
    void AddToSequence(BeatTimeEvent const& newEvent) {
        _events.push_back(newEvent);
        SortEventsByTime();
    }

    void Update(float const dt) override {
        if (_events.empty()) {
            return;
        }

        // Queue up events up to 1 beat before they should be played.
        double currentBeatTime = _beatClock->GetBeatTime();
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
                    long tickTime = _beatClock->BeatTimeToTickTime(absBeatTime);
                    audio::Event e = b_e._e;
                    e.timeInTicks = tickTime;
                    _audio->AddEvent(e);
                } else {
                    // not time yet. quit.
                    break;
                }
            }
            if (_currentIx < _events.size()) {
                break;
            } else {
                _currentIx = -1;
                double lastEventTime = _events.back()._beatTime;
                assert(lastEventTime > 0.0);  // avoids infinite loop
                _currentLoopStartBeatTime += lastEventTime;
            }
        }
    }

    void EditModeUpdate(float const dt) override {
        if (_editUpdateEnabled) {
            Update(dt);
        }
    }

    bool DrawImGui() override {
        ImGui::Checkbox("Update in edit mode##", &_editUpdateEnabled);

        if (ImGui::Button("Add Event##")) {
            _events.emplace_back();
        }

        char headerName[128];
        for (int i = 0; i < _events.size(); ++i) {
            ImGui::PushID(i);
            sprintf(headerName, "%s##Header", audio::EventTypeToString(_events[i]._e.type));
            if (ImGui::CollapsingHeader(headerName)) {
                if (ImGui::Button("Remove##")) {
                    _events.erase(_events.begin() + i);
                    --i;
                } else {
                    ImGui::InputScalar("Beat time##", ImGuiDataType_Double, &_events[i]._beatTime, /*step=*/nullptr, /*???*/nullptr, "%f");
                    audio::EventDrawImGuiNoTime(_events[i]._e);
                }
            }
            ImGui::PopID();
        }

        // Keep them sorted.
        // SortEventsByTime();

        return false;
    }

    void Save(ptree& pt) const override {
        ptree eventsPt;
        for (BeatTimeEvent const& b_e : _events) {
            serial::SaveInNewChildOf(eventsPt, "beat_event", b_e);
        }
        pt.add_child("beat_events", eventsPt);
    }

    void Load(ptree const& pt) override {
        for (auto const& item : pt.get_child("beat_events")) {
            _events.emplace_back();
            BeatTimeEvent &b_e = _events.back();
            b_e.Load(item.second);
        }
    }

    // Serialized
    std::vector<BeatTimeEvent> _events;

    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    bool _editUpdateEnabled = false;
    int _currentIx = -1;
    double _currentLoopStartBeatTime = -1.0;
};