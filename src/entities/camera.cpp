#include "entities/camera.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "renderer.h"

void CameraEntity::Init(GameManager& g) {
    _camera = &g._scene->_camera;
    if (g._editMode) {
        // If in edit mode, initialize the debug camera location to this location.
        _camera->_transform = _transform;
        _camera->_projectionType = renderer::Camera::ProjectionType::Orthographic;
    } else if (!_followEntityName.empty()) {
        ne::BaseEntity* followEntity = g._neEntityManager->FindEntityByName(_followEntityName);
        if (followEntity != nullptr) {
            _followEntityId = followEntity->_id;
            _desiredTargetToCameraOffset = _transform.GetPos() - followEntity->_transform.GetPos();
        } else {
            printf("WARNING: CameraEntity could not find follow entity named \"%s\"", _followEntityName.c_str());
        }
    }
}

void CameraEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        // In this case, let Editor control the camera.
        return;
    }

    if (_followEntityId.IsValid()) {
        if (ne::BaseEntity* followEntity = g._neEntityManager->GetEntity(_followEntityId)) {
            Vec3 cameraPos = _transform.GetPos();
            Vec3 targetPos = followEntity->_transform.GetPos();
            Vec3 targetToCameraOffset = cameraPos - targetPos;
            float k = _trackingFactor * 60.f;  // TOD0: 60 is the expected fps right???
            Vec3 diff = _desiredTargetToCameraOffset - targetToCameraOffset;
            Vec3 newOffset = targetToCameraOffset + k * dt * diff;
            _transform.SetTranslation(targetPos + newOffset);
        } else {
            _followEntityId = ne::EntityId();
        }
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

void CameraEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("ortho", _ortho);
    pt.PutString("follow_entity_name", _followEntityName.c_str());
    pt.PutFloat("tracking_factor", _trackingFactor);
}

void CameraEntity::LoadDerived(serial::Ptree pt) {
    _ortho = false;
    pt.TryGetBool("ortho", &_ortho);
    pt.TryGetString("follow_entity_name", &_followEntityName);
    pt.TryGetFloat("tracking_factor", &_trackingFactor);
}
