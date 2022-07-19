#pragma once

#include "component.h"
#include "matrix.h"

class TransformComponent;
namespace renderer {
    class Camera;
}

class CameraComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Camera; }
    CameraComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    virtual void Update(float dt) override;
    virtual void EditModeUpdate(float dt) override;
    virtual ~CameraComponent() override {}

    std::weak_ptr<TransformComponent> _transform;
    renderer::Camera* _camera = nullptr;
};