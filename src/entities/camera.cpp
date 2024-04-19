#include "entities/camera.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "renderer.h"

void CameraEntity::InitDerived(GameManager& g) {
    _camera = &g._scene->_camera;
    if (g._editMode) {
        // If in edit mode, initialize the debug camera location to this location.
        _camera->_transform = _transform.Mat4NoScale();
        _camera->_projectionType = _ortho ? renderer::Camera::ProjectionType::Orthographic : renderer::Camera::ProjectionType::Perspective;
    } else if (!_followEntityName.empty()) {
        ne::BaseEntity* followEntity = g._neEntityManager->FindEntityByName(_followEntityName);
        if (followEntity != nullptr) {
            _followEntityId = followEntity->_id;
            _desiredTargetToCameraOffset = _transform.GetPos() - followEntity->_transform.GetPos();
        } else {
            printf("WARNING: CameraEntity could not find follow entity named \"%s\"", _followEntityName.c_str());
        }
    }

    _trackingFactor = _initialTrackingFactor;
}

void CameraEntity::UpdateDerived(GameManager& g, float dt) {
    _camera->_fovyRad = _fovyDeg * kPi / 180.f;
    _camera->_zNear = _zNear;
    _camera->_zFar = _zFar;
    _camera->_width = _width;
    
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
    Vec3 diffDir = diff;
    float diffLength = diffDir.Normalize();
    diff += diffDir * 0.2f;  // overshoot
    Vec3 newOffset = targetToCameraOffset + k * dt * diff;
    Vec3 newDiff = _desiredTargetToCameraOffset - newOffset;
    if (Vec3::Dot(newDiff, diff) < 0.f) {
        // We've crossed the desired offset
        newOffset = _desiredTargetToCameraOffset;
    }
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
    if (ImGui::Checkbox("Ortho", &_ortho)) {
        _camera->_projectionType = _ortho ? renderer::Camera::ProjectionType::Orthographic : renderer::Camera::ProjectionType::Perspective;
    }
    if (ImGui::Button("Move Debug Camera to This")) {
        _camera->_transform = _transform.Mat4NoScale();
    }
    if (ImGui::Button("Move This to Debug Camera")) {
        Transform t;
        t.SetFromMat4(_camera->_transform);
        SetAllTransforms(t);
    }
    ImGui::DragFloat("Fovy (deg)", &_fovyDeg, /*v_speed=*/1.f, /*v_min=*/10.f, /*v_max=*/120.f);
    ImGui::InputFloat("znear", &_zNear);
    ImGui::InputFloat("zfar", &_zFar);
    ImGui::InputFloat("width", &_width);
    return ImGuiResult::Done;
}

void CameraEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("ortho", _ortho);
    pt.PutString("follow_entity_name", _followEntityName.c_str());
    pt.PutFloat("tracking_factor", _initialTrackingFactor);
    pt.PutFloat("fovy_deg", _fovyDeg);
    pt.PutFloat("z_near", _zNear);
    pt.PutFloat("z_far", _zFar);
    pt.PutFloat("width", _width);
}

void CameraEntity::LoadDerived(serial::Ptree pt) {
    _ortho = false;
    pt.TryGetBool("ortho", &_ortho);
    pt.TryGetString("follow_entity_name", &_followEntityName);
    pt.TryGetFloat("tracking_factor", &_initialTrackingFactor);
    _fovyDeg = 45.f;
    pt.TryGetFloat("fovy_deg", &_fovyDeg);
    pt.TryGetFloat("z_near", &_zNear);
    pt.TryGetFloat("z_far", &_zFar);
    pt.TryGetFloat("width", &_width);
}

void CameraEntity::JumpToPosition(Vec3 const& p) {
    Vec3 newPos = p + _desiredTargetToCameraOffset;
    _transform.SetPos(newPos);
}
