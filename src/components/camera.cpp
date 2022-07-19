#include "components/camera.h"

#include "serial.h"
#include "entity.h"
#include "game_manager.h"
#include "components/transform.h"
#include "renderer.h"

bool CameraComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    _mgr = g._scene;
    _mgr->AddCamera(e.FindComponentOfType<CameraComponent>());
    return !_transform.expired();
}

Mat4 CameraComponent::GetViewMatrix() const {
    auto transform = _transform.lock();
    Vec3 p = transform->GetWorldPos();
    Vec3 forward = -transform->GetWorldZAxis();  // Z-axis points backward
    Vec3 up = transform->GetWorldYAxis();
    return Mat4::LookAt(p, p + forward, up);
}