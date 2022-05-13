#pragma once

#include "component.h"

class BeatClock;
namespace audio {
    class Context;
}

class BeepOnHitComponent : public Component {
public:
    BeepOnHitComponent(TransformComponent* t, audio::Context* audio, BeatClock const* beatClock)
        : _t(t)
        , _audio(audio)
        , _beatClock(beatClock) {
    }

    void OnHit() {
        _wasHit = true;
    }

    virtual void Update(float const dt) override;

    TransformComponent* _t = nullptr;
    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    double _lastScheduledEvent = -1.0;
    bool _wasHit = false;
};