#pragma once

#include "component.h"

class BeatClock;
namespace audio {
    class Context;
}

class BeepOnHitComponent : public Component {
public:
    virtual ComponentType Type() override { return ComponentType::BeepOnHit; }
    BeepOnHitComponent() {}    
    virtual void ConnectComponents(Entity& e, GameManager& g) override;

    void OnHit() {
        _wasHit = true;
    }

    virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    TransformComponent* _t = nullptr;
    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    double _lastScheduledEvent = -1.0;
    bool _wasHit = false;
};