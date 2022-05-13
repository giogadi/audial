#pragma once

#include <vector>

#include "component.h"
#include "audio_util.h"
#include "beat_clock.h"

// Does NOT loop yet.
class SequencerComponent : public Component {
public:
    SequencerComponent(audio::Context* audio, BeatClock const* beatClock)
        : _audio(audio)
        , _beatClock(beatClock) {}
    
    // Keeps event sorted by time
    void AddToSequence(audio::Event const& newEvent) {
        _events.push_back(newEvent);
        std::sort(_events.begin(), _events.end(), [](audio::Event const& e1, audio::Event const& e2) {
            return e1.timeInTicks < e2.timeInTicks;
        });
    }
    
    void Update(float const dt) {        
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

    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    std::vector<audio::Event> _events;
    int _currentIx = 0;
};