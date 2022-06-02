#pragma once

#include "component.h"

class RigidBodyComponent;

class OrbitableComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Orbitable; }
    OrbitableComponent() {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;

    // static void OnHit(
    //     std::weak_ptr<OrbitableComponent> thisComp, std::weak_ptr<RigidBodyComponent> other);

    // virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    // virtual bool DrawImGui() override;

    std::weak_ptr<TransformComponent> _t;
    std::weak_ptr<RigidBodyComponent> _rb;    
    // bool _wasHit = false;
};