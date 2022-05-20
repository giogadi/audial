#pragma once

#include "component.h"

class BeatClock;
namespace audio {
    class Context;
}
class RigidBodyComponent;

class BeepOnHitComponent : public Component {
public:
    virtual ComponentType Type() override { return ComponentType::BeepOnHit; }
    BeepOnHitComponent() {}    
    virtual void ConnectComponents(Entity& e, GameManager& g) override;

    void OnHit(RigidBodyComponent* other);

    virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    TransformComponent* _t = nullptr;
    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    int _synthChannel = 0;
    std::vector<int> _midiNotes;
    double _lastScheduledEvent = -1.0;
    bool _wasHit = false;
};