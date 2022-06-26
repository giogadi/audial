#pragma once

#include "component.h"

class RigidBodyComponent;

class OrbitableComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Orbitable; }
    OrbitableComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    virtual void Destroy() override {}

    std::weak_ptr<TransformComponent> _t;
    std::weak_ptr<RigidBodyComponent> _rb;
};