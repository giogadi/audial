#include "camera_controller.h"

#include "imgui/imgui.h"

#include "components/transform.h"
#include "entity_manager.h"
#include "game_manager.h"

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

    _desiredTargetToCameraOffset = _transform.lock()->GetWorldPos() - _target.lock()->GetWorldPos();

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
    std::shared_ptr<TransformComponent const> target = _target.lock();
    if (target == nullptr) {
        return;
    }
    Vec3 cameraPos = _transform.lock()->GetWorldPos();
    Vec3 targetPos = target->GetWorldPos();
    Vec3 targetToCameraOffset = cameraPos - targetPos;

    float k = _trackingFactor * 60.f;
    Vec3 newOffset = targetToCameraOffset + k * dt * (_desiredTargetToCameraOffset - targetToCameraOffset);
    _transform.lock()->SetWorldPos(targetPos + newOffset);
}

void CameraControllerComponent::SetTarget(std::shared_ptr<TransformComponent const> const& newTarget) {
    _target = newTarget;
    // FOR NOW, we just keep the previous target-to-camera offset. Later we might want to specify that here too.
}

void CameraControllerComponent::Save(serial::Ptree pt) const {
    pt.PutFloat("tracking_factor", _trackingFactor);
    pt.PutString("target_entity_name", _targetName.c_str());
}

void CameraControllerComponent::Load(serial::Ptree pt) {
    _trackingFactor = pt.GetFloat("tracking_factor");
    _targetName = pt.GetString("target_entity_name");
}