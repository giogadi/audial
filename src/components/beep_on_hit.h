#pragma once

#include <vector>
#include <algorithm>

#include "component.h"
#include "beat_clock.h"
#include "audio.h"

struct Hitbox {
    glm::vec3 _min;
    glm::vec3 _max;
};
// TODO: maybe make this a callback instead of a class to avoid virtual call
class HitListener {
public:
    virtual void OnHit(Hitbox const& hitbox) = 0;
};
class HitManager {
public:
    void AddListener(HitListener* l) { _listeners.push_back(l); }
    void RemoveListener(HitListener* l) {
        _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), l));
    }
    
    void Hit(Hitbox const& hitbox) {
        for (HitListener* l : _listeners) {
            l->OnHit(hitbox);
        }
    }

    std::vector<HitListener*> _listeners;
};

class BeepOnHitComponent : public Component {
public:
    struct BeepHitListener : public HitListener {
        BeepHitListener(TransformComponent const* t)
            : _t(t) {}
        virtual void OnHit(Hitbox const& hitbox) override {
            glm::vec3 hurtMin(_t->GetPos() - glm::vec3(0.5f,0.5f,0.5f));
            glm::vec3 hurtMax(_t->GetPos() + glm::vec3(0.5f,0.5f,0.5f));
            for (int i = 0; i < 3; ++i) {
                if (hurtMin[i] > hitbox._max[i] ||
                    hurtMax[i] < hitbox._min[i]) {
                    return;
                }
            }
            _hit = true;
        }
        bool _hit = false;
        TransformComponent const* _t = nullptr;
    };

    BeepOnHitComponent(TransformComponent* t, HitManager* hitMgr, audio::Context* audio, BeatClock const* beatClock)
        : _t(t)
        , _hitMgr(hitMgr)
        , _hitListener(t)
        , _audio(audio)
        , _beatClock(beatClock) {
        _hitMgr->AddListener(&_hitListener);
    }
    virtual void Destroy() override {
        _hitMgr->RemoveListener(&_hitListener);
    }

    virtual void Update(float const dt) override { 
        if (_hitListener._hit) {
            double beatTime = _beatClock->GetBeatTime();
            double denom = 0.25;
            double noteTime = BeatClock::GetNextBeatDenomTime(beatTime, denom);
            double noteOffTime = noteTime + 0.5 * denom;
            if (noteOffTime > _lastScheduledEvent) {
                _lastScheduledEvent = noteOffTime;
                audio::Event e;
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
        _hitListener._hit = false;
    }

    TransformComponent* _t = nullptr;
    HitManager* _hitMgr = nullptr;
    BeepHitListener _hitListener;
    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    double _lastScheduledEvent = -1.0;
};