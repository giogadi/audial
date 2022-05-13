#pragma once

#include <vector>
#include <algorithm>

#include "glm/gtx/norm.hpp"

#include "component.h"
#include "beat_clock.h"
#include "audio.h"

struct Hitbox {
    glm::vec3 _min;
    glm::vec3 _max;
};

struct HitSphere {
    glm::vec3 _center;
    float _radius;
};
// TODO: maybe make this a callback instead of a class to avoid virtual call
class HitListener {
public:
    // Returns position of thing that got hit
    virtual bool OnHit(Hitbox const& hitbox, Hitbox& hurtbox) = 0;
    virtual bool OnHit(HitSphere const& hitSphere, HitSphere& hurtSphere) = 0;
};
class HitManager {
public:
    void AddListener(HitListener* l) { _listeners.push_back(l); }
    void RemoveListener(HitListener* l) {
        _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), l));
    }
    
    void HitAndStore(Hitbox const& attackHitbox, std::vector<Hitbox>* hits) {
        Hitbox hurtbox;
        for (HitListener* l : _listeners) {            
            if (l->OnHit(attackHitbox, hurtbox)) {
                hits->push_back(hurtbox);
            }
        }
    }
    void HitAndStore(HitSphere const& attackHitSphere, std::vector<HitSphere>* hits) {
        HitSphere hurtSphere;
        for (HitListener* l : _listeners) {            
            if (l->OnHit(attackHitSphere, hurtSphere)) {
                hits->push_back(hurtSphere);
            }
        }
    }

    std::vector<HitListener*> _listeners;
};

class BeepOnHitComponent : public Component {
public:
    struct BeepHitListener : public HitListener {
        BeepHitListener(TransformComponent const* t, float r)
            : _t(t)
            , _hitSphereRadius(r) {}
        virtual bool OnHit(Hitbox const& hitbox, Hitbox& hurtbox) override {
            glm::vec3 hurtMin(_t->GetPos() - glm::vec3(_hitSphereRadius));
            glm::vec3 hurtMax(_t->GetPos() + glm::vec3(_hitSphereRadius));
            for (int i = 0; i < 3; ++i) {
                if (hurtMin[i] > hitbox._max[i] ||
                    hurtMax[i] < hitbox._min[i]) {
                    _hit = false;
                    return false;
                }
            }
            _hit = true;
            hurtbox._min = hurtMin;
            hurtbox._max = hurtMax;
            return true;
        }
        virtual bool OnHit(HitSphere const& theirHitSphere, HitSphere& hurtSphere) override {
            float sumRad = _hitSphereRadius + theirHitSphere._radius;
            bool hit = glm::length2(_t->GetPos() - theirHitSphere._center) <= sumRad*sumRad;
            if (hit) {
                hurtSphere._center = _t->GetPos();
                hurtSphere._radius = _hitSphereRadius;
            }
            return hit;
        }

        bool _hit = false;
        TransformComponent const* _t = nullptr;
        float _hitSphereRadius = 0.f;
    };

    BeepOnHitComponent(TransformComponent* t, float radius, HitManager* hitMgr, audio::Context* audio, BeatClock const* beatClock)
        : _t(t)
        , _hitMgr(hitMgr)
        , _hitListener(t, radius)
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
        _hitListener._hit = false;
    }

    TransformComponent* _t = nullptr;
    HitManager* _hitMgr = nullptr;
    BeepHitListener _hitListener;
    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    double _lastScheduledEvent = -1.0;
};