#pragma once

#include "component.h"

class RigidBodyComponent;

class DamageComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Damage; }
    DamageComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    virtual void Update(float dt) override;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    static void OnHit(
        std::weak_ptr<DamageComponent> damageComp, std::weak_ptr<RigidBodyComponent> other);

    int _hp = -1;
    std::weak_ptr<RigidBodyComponent> _rb;
    EntityId _entityId = EntityId::InvalidId();
    EntityManager* _entityManager = nullptr;
    bool _wasHit = false;
};