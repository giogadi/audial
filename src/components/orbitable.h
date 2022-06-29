#pragma once

#include "component.h"

class RigidBodyComponent;
class ScriptAction;

class OrbitableComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Orbitable; }
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    virtual void Destroy() override {}

    virtual void Save(ptree& pt) const override;
    virtual void Load(ptree const& pt) override;

    virtual bool DrawImGui() override;

    void OnLeaveOrbit(GameManager& g);

    // serialize
    std::vector<std::unique_ptr<ScriptAction>> _onLeaveActions;

    std::weak_ptr<TransformComponent> _t;
    std::weak_ptr<RigidBodyComponent> _rb;
};