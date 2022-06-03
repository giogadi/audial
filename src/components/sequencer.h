#pragma once

#include <vector>

#include "component.h"
#include "audio_util.h"
#include "beat_clock.h"
#include "audio.h"
#include "audio_event_imgui.h"

// Does NOT loop yet.
class SequencerComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Sequencer; }
    SequencerComponent() {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override {
        _audio = g._audioContext;
        _beatClock = g._beatClock;
        return true;
    }

    void SortEventsByTime() {
        std::sort(_events.begin(), _events.end(), [](audio::Event const& e1, audio::Event const& e2) {
            return e1.timeInTicks < e2.timeInTicks;
        });
    }
    
    // Keeps event sorted by time
    void AddToSequence(audio::Event const& newEvent) {
        _events.push_back(newEvent);
        SortEventsByTime();
    }
    
    void Update(float const dt) override {        
        if (_events.empty()) {
            return;
        }
        // Queue up events up to 1 beat before they should be played.
        unsigned long maxTickTime = _beatClock->BeatTimeToTickTime(_beatClock->GetBeatTime() + 1.0);
        for (; _currentIx < _events.size(); ++_currentIx) {
            audio::Event const& e = _events[_currentIx];
            if (e.timeInTicks <= maxTickTime) {
                if (!_audio->_eventQueue.try_push(e)) {
                    std::cout << "Sequencer failed to push event!" << std::endl;
                }
            } else {
                break;
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
            sprintf(headerName, "%s##Header", audio::EventTypeToString(_events[i].type));
            if (ImGui::CollapsingHeader(headerName)) {
                audio::EventDrawImGuiBeatTime(_events[i], *_beatClock);        
            }
            ImGui::PopID();
        }

        // Keep them sorted.
        SortEventsByTime();

        return false;
    }

    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    std::vector<audio::Event> _events;
    bool _editUpdateEnabled = true;
    int _currentIx = 0;
};