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
        BoundMesh const* mesh = meshIter->second.get();
        _scene = g._scene;
        std::pair<VersionId, renderer::ModelInstance*> item = _scene->AddModelInstance();
        _modelInstanceId = item.first;
        item.second->Set(_transform.lock()->GetWorldMat4(), mesh, _color);
        return true;
    } else {
        return false;
    }
}

void ModelComponent::Destroy() {
    if (_scene) {
        _scene->RemoveModelInstance(_modelInstanceId);
    }    
}

void ModelComponent::EditDestroy() {
    Destroy();
}

void ModelComponent::Update(float dt) {
    renderer::ModelInstance* m = _scene->GetModelInstance(_modelInstanceId);
    m->_transform = _transform.lock()->GetWorldMat4();
    m->_color = _color;
}

void ModelComponent::EditModeUpdate(float dt) {
    Update(dt);
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