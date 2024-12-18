#pragma once

#include "component.h"

#include <vector>
#include <memory>

class RigidBodyComponent;
class ScriptAction;
class TransformComponent;
class DamageComponent;

class OrbitableComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Orbitable; }
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    virtual void Save(serial::Ptree pt) const override;
    virtual void Load(serial::Ptree pt) override;

    virtual bool DrawImGui() override;

    void OnLeaveOrbit(GameManager& g);

    // serialize
    std::vector<std::unique_ptr<ScriptAction>> _onLeaveActions;
    std::string _recorderName;

    std::weak_ptr<TransformComponent> _t;
    std::weak_ptr<RigidBodyComponent> _rb;
    std::weak_ptr<DamageComponent> _damage;
};