#pragma once

#include "component.h"

class CameraControllerComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::CameraController; }
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    virtual void Update(float dt) override;
    virtual bool DrawImGui() override;

    virtual void Save(ptree& pt) const override;
    virtual void Load(ptree const& pt) override;

    // Serialized
    float _trackingFactor = 0.05f;
    // NOTE: this is currently only read during ConnectComponents. Setting it
    // will not cause the target to automatically update.
    std::string _targetName;

    std::weak_ptr<TransformComponent> _transform;
    std::weak_ptr<TransformComponent> _target;
    Vec3 _desiredTargetToCameraOffset;  // Must be initialized in ConnectComponents
};