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

    void OnHit(EntityId other);

    void Save(serial::Ptree pt) const override;
    void Load(serial::Ptree pt) override;

    // Serialize
    int _hp = -1;

    EntityId _entityId = EntityId::InvalidId();
    EntityManager* _entityManager = nullptr;
    bool _wasHit = false;
};