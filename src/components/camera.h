#pragma once

#include "component.h"
#include "matrix.h"

class TransformComponent;
class SceneManager;

class CameraComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Camera; }
    CameraComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    virtual ~CameraComponent() {}

    Mat4 GetViewMatrix() const;

    std::weak_ptr<TransformComponent> _transform;
    SceneManager* _mgr = nullptr;
};