#include "activator.h"

#include "imgui/imgui.h"

#include "beat_clock.h"

bool ActivatorComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _gameManager = &g;
    _beatClock = g._beatClock;
    return true;
}

bool ActivatorComponent::DrawImGui() {
    char inputName[128];
    assert(_entityName.length() < 128);
    strcpy(inputName, _entityName.c_str());
    ImGui::InputText("Entity name##", inputName, 128);
    _entityName = inputName;

    ImGui::InputScalar(
        "Activation beat time##", ImGuiDataType_Double, &_activationBeatTime, /*step=*/nullptr);

    return false;
}

void ActivatorComponent::Update(float dt) {
    if (_haveActivated) {
        return;
    }
    if (_absoluteActivationBeatTime < 0.0) {
        _absoluteActivationBeatTime = std::ceil(_beatClock->GetBeatTime()) + _activationBeatTime;
    }
    if (_beatClock->GetBeatTime() >= _absoluteActivationBeatTime) {
        EntityId toActivateId = _gameManager->_entityManager->FindInactiveEntityByName(_entityName.c_str());
        _gameManager->_entityManager->ActivateEntity(toActivateId, *_gameManager);
        _haveActivated = true;
    }
}

void ActivatorComponent::Save(ptree& pt) const {
    pt.put("entity_name", _entityName);
    pt.put("beat_time", _activationBeatTime);
}

void ActivatorComponent::Load(ptree const& pt) {
    _entityName = pt.get<std::string>("entity_name");
    _activationBeatTime = pt.get<double>("beat_time");
}