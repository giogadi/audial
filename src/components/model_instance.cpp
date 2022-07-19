#include "components/model_instance.h"

#include "imgui/imgui.h"

#include "model.h"
#include "renderer.h"
#include "serial.h"
#include "entity.h"
#include "game_manager.h"
#include "resource_manager.h"
#include "components/transform.h"

bool ModelComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    if (_transform.expired()) {
        return false;
    }
    ModelManager* modelManager = g._modelManager;
    auto meshIter = modelManager->_modelMap.find(_modelId);
    if (meshIter != modelManager->_modelMap.end()) {
        _mesh = meshIter->second.get();
        _mgr = g._scene;
        _mgr->AddModel(e.FindComponentOfType<ModelComponent>());
        return true;
    } else {
        return false;
    }
}

bool ModelComponent::DrawImGui() {
    char inputStr[128];
    assert(_modelId.length() < 128);
    strcpy(inputStr, _modelId.c_str());
    bool needReconnect = ImGui::InputText("Model ID", inputStr, 128);
    _modelId = inputStr;
    ImGui::ColorEdit4("Color", _color._data);
    return needReconnect;
}

void ModelComponent::Save(ptree& pt) const {
    pt.put("model_id", _modelId);
    serial::SaveInNewChildOf(pt, "color", _color);
}

void ModelComponent::Load(ptree const& pt) {
    _modelId = pt.get<std::string>("model_id");
    _color.Load(pt.get_child("color"));
}