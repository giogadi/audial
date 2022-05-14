#include "beep_on_hit.h"

#include "beat_clock.h"
#include "audio.h"

void BeepOnHitComponent::Update(float dt) {
    if (!_wasHit) {
        return;
    }
    _wasHit = false;

    double beatTime = _beatClock->GetBeatTime();
    double denom = 0.25;
    double noteTime = BeatClock::GetNextBeatDenomTime(beatTime, denom);
    double noteOffTime = noteTime + 0.5 * denom;
    if (noteOffTime > _lastScheduledEvent) {
        _lastScheduledEvent = noteOffTime;
        audio::Event e;
        e.channel = 0;
        e.type = audio::EventType::NoteOn;
        e.midiNote = 69;
        e.timeInTicks = _beatClock->BeatTimeToTickTime(noteTime);
        if (_audio->_eventQueue.try_push(e)) {
            e.type = audio::EventType::NoteOff;
            e.midiNote = 69;
            e.timeInTicks = _beatClock->BeatTimeToTickTime(noteOffTime);
            if (!_audio->_eventQueue.try_push(e)) {
                std::cout << "Failed to push note off" << std::endl;
            }
        } else {
            std::cout << "Failed to push note on" << std::endl;
        }

        e.type = audio::EventType::NoteOn;
        e.midiNote = 73;
        e.timeInTicks = _beatClock->BeatTimeToTickTime(noteTime);
        if (_audio->_eventQueue.try_push(e)) {
            e.type = audio::EventType::NoteOff;
            e.midiNote = 73;
            e.timeInTicks = _beatClock->BeatTimeToTickTime(noteOffTime);
            if (!_audio->_eventQueue.try_push(e)) {
                std::cout << "Failed to push note off" << std::endl;
            }
        } else {
            std::cout << "Failed to push note on" << std::endl;
        }
    }
}