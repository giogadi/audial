#pragma once

#include "component.h"

class RigidBodyComponent;

class HitCounterComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::HitCounter; }
    HitCounterComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    static void OnHit(
        std::weak_ptr<HitCounterComponent> beepComp, std::weak_ptr<RigidBodyComponent> other);

    virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    std::weak_ptr<TransformComponent> _t;
    std::weak_ptr<RigidBodyComponent> _rb;
    bool _wasHit = false;
    int _hitsRemaining = 0;
};