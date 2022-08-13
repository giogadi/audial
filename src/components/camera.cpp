#include "components/camera.h"

#include "imgui/imgui.h"

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
    _camera->_transform = _transform.lock()->GetWorldMat4();
}

void CameraComponent::EditModeUpdate(float dt) {
    // In edit mode, we don't drive the renderer's camera using this component.
}

bool CameraComponent::DrawImGui() {
    if (ImGui::Button("Move Debug Camera to This")) {
        _camera->_transform = _transform.lock()->GetWorldMat4();
    }
    if (ImGui::Button("Move This to Debug Camera")) {
        _transform.lock()->SetWorldMat4(_camera->_transform);
    }
    return false;
}