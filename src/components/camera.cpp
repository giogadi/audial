#include "components/camera.h"

#include "serial.h"
#include "entity.h"
#include "game_manager.h"
#include "components/transform.h"
#include "renderer.h"

bool CameraComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    _camera = &g._scene->_camera;
    return !_transform.expired();
}

void CameraComponent::Update(float dt) {
    // _camera->Set(_transform.lock()->GetWorldMat4(), 45.f * kPi / 180.f, 0.1f, 100.f);
    _camera->_transform = _transform.lock()->GetWorldMat4();
}

void CameraComponent::EditModeUpdate(float dt) {
    Update(dt);
}