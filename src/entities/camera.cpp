#include "entities/camera.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "renderer.h"

void CameraEntity::Init(GameManager& g) {
    _camera = &g._scene->_camera;
    if (g._editMode) {
        // If in edit mode, initialize the debug camera location to this location.
        _camera->_transform = _transform;
    }    
}

void CameraEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        // In this case, let Editor control the camera.
        return;
    }
    _camera->_transform = _transform;
    _camera->_projectionType = 
        _ortho ? renderer::Camera::ProjectionType::Orthographic :
            renderer::Camera::ProjectionType::Perspective;
}

ne::Entity::ImGuiResult CameraEntity::ImGuiDerived(GameManager& g) {
    if (ImGui::Button("Move Debug Camera to This")) {
        _camera->_transform = _transform;
    }
    if (ImGui::Button("Move This to Debug Camera")) {
        _transform = _camera->_transform;
    }
    return ImGuiResult::Done;
}