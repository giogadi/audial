#include "entities/camera.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "renderer.h"

void CameraEntity::InitDerived(GameManager& g) {
    _camera = &g._scene->_camera;
    if (g._editMode) {
        // If in edit mode, initialize the debug camera location to this location.
        _camera->_transform = _transform.Mat4NoScale();
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

    Vec3 targetPos = _transform.GetPos();   
    if (_followEntityId.IsValid()) {
        if (ne::BaseEntity* followEntity = g._neEntityManager->GetEntity(_followEntityId)) {
            targetPos = followEntity->_transform.GetPos();
        } else {
            _followEntityId = ne::EntityId();
        }
    }

    Vec3 const& cameraPos = _transform.GetPos();
    Vec3 targetToCameraOffset = cameraPos - targetPos;
    float k = _trackingFactor * 60.f;  // TOD0: 60 is the expected fps right???
    Vec3 diff = _desiredTargetToCameraOffset - targetToCameraOffset;
    Vec3 newOffset = targetToCameraOffset + k * dt * diff;
    Vec3 newPos = targetPos + newOffset;

    // Fade in the constraints
    if (_constraintBlend == 0.f) {
        _constraintBlend = 0.f;
    }

    float constexpr kConstraintFactor = 0.01f;
    _constraintBlend += kConstraintFactor * 60.f * dt * (1.f - _constraintBlend);
    {
        Vec3 clampedPos = newPos;
        if (_minX.has_value()) {
            clampedPos._x = std::max(clampedPos._x, *_minX);
        }
        if (_minZ.has_value()) {
            clampedPos._z = std::max(clampedPos._z, *_minZ);
        }
        if (_maxX.has_value()) {
            clampedPos._x = std::min(clampedPos._x, *_maxX);
        }
        if (_maxZ.has_value()) {
            clampedPos._z = std::min(clampedPos._z, *_maxZ);
        }
        Vec3 diff = clampedPos - newPos;
        Vec3 newDiff = _constraintBlend * diff;
        newPos += newDiff;
        _transform.SetTranslation(newPos);
    }         
    
    _camera->_transform = _transform.Mat4NoScale();
    _camera->_projectionType = 
        _ortho ? renderer::Camera::ProjectionType::Orthographic :
            renderer::Camera::ProjectionType::Perspective;
}

ne::Entity::ImGuiResult CameraEntity::ImGuiDerived(GameManager& g) {
    if (ImGui::Button("Move Debug Camera to This")) {
        _camera->_transform = _transform.Mat4NoScale();
    }
    if (ImGui::Button("Move This to Debug Camera")) {
        _transform.SetFromMat4(_camera->_transform);
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
