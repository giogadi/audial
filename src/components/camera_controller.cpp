#include "camera_controller.h"

#include "imgui/imgui.h"

bool CameraControllerComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    if (_transform.expired()) {
        return false;
    }
    EntityId targetEntityId = g._entityManager->FindActiveEntityByName(_targetName.c_str());
    if (!targetEntityId.IsValid()) {
        return false;
    }
    Entity* targetEntity = g._entityManager->GetEntity(targetEntityId);
    _target = targetEntity->FindComponentOfType<TransformComponent>();
    if (_target.expired()) {
        return false;
    }

    _desiredTargetToCameraOffset = _transform.lock()->GetPos() - _target.lock()->GetPos();

    return true;
}

bool CameraControllerComponent::DrawImGui() {
    char inputStr[128];
    assert(_targetName.length() < 128);
    strcpy(inputStr, _targetName.c_str());
    bool needReconnect = ImGui::InputText("Target EntityId", inputStr, 128);
    _targetName = inputStr;
    ImGui::InputScalar("Tracking factor", ImGuiDataType_Float, &_trackingFactor, /*step=*/nullptr);
    return needReconnect;
}

// TODO: make this framerate independent!!!
void CameraControllerComponent::Update(float dt) {
    Vec3 cameraPos = _transform.lock()->GetPos();
    Vec3 targetPos = _target.lock()->GetPos();
    Vec3 targetToCameraOffset = cameraPos - targetPos;

    Vec3 newOffset = targetToCameraOffset + _trackingFactor * (_desiredTargetToCameraOffset - targetToCameraOffset);
    _transform.lock()->SetPos(targetPos + newOffset);
}

void CameraControllerComponent::Save(ptree& pt) const {
    pt.put("tracking_factor", _trackingFactor);
    pt.put("target_entity_name", _targetName);
}

void CameraControllerComponent::Load(ptree const& pt) {
    _trackingFactor = pt.get<float>("tracking_factor");
    _targetName = pt.get<std::string>("target_entity_name");
}