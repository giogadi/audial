#pragma once

#include "component.h"

class RigidBodyComponent;

class DamageComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Damage; }
    DamageComponent() {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;

    virtual void Update(float dt) override;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    static void OnHit(
        std::weak_ptr<DamageComponent> damageComp, std::weak_ptr<RigidBodyComponent> other);

    int _hp = -1;
    std::weak_ptr<RigidBodyComponent> _rb;
    // DO NOT DEREFERENCE THE ENTITY! Only used to tell manager which entity to delete.
    Entity* _entity = nullptr;
    EntityManager* _entityManager = nullptr;
    bool _wasHit = false;
};