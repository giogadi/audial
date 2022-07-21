#include "components/model_instance.h"

#include "imgui/imgui.h"

#include "mesh.h"
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
    MeshManager* meshManager = g._meshManager;
    auto meshIter = meshManager->_meshMap.find(_meshId);
    if (meshIter != meshManager->_meshMap.end()) {
        BoundMeshPNU const* mesh = meshIter->second.get();
        _scene = g._scene;
        std::pair<VersionId, renderer::ColorModelInstance*> item = _scene->AddColorModelInstance();
        _modelInstanceId = item.first;
        item.second->Set(_transform.lock()->GetWorldMat4(), mesh, _color);
        return true;
    } else {
        return false;
    }
}

void ModelComponent::Destroy() {
    if (_scene) {
        _scene->RemoveColorModelInstance(_modelInstanceId);
    }    
}

void ModelComponent::EditDestroy() {
    Destroy();
}

void ModelComponent::Update(float dt) {
    renderer::ColorModelInstance* m = _scene->GetColorModelInstance(_modelInstanceId);
    m->_transform = _transform.lock()->GetWorldMat4();
    m->_color = _color;
}

void ModelComponent::EditModeUpdate(float dt) {
    Update(dt);
}

bool ModelComponent::DrawImGui() {
    char inputStr[128];
    assert(_meshId.length() < 128);
    strcpy(inputStr, _meshId.c_str());
    bool needReconnect = ImGui::InputText("Mesh ID", inputStr, 128);
    _meshId = inputStr;
    ImGui::ColorEdit4("Color", _color._data);
    return needReconnect;
}

void ModelComponent::Save(ptree& pt) const {
    pt.put("mesh_id", _meshId);
    serial::SaveInNewChildOf(pt, "color", _color);
}

void ModelComponent::Load(ptree const& pt) {
    auto const& maybeMeshId = pt.get_optional<std::string>("mesh_id");
    if (maybeMeshId.has_value()) {
        _meshId = maybeMeshId.value();
    } else {
        // old version used "model_id"
        _meshId = pt.get<std::string>("model_id");
    }
    _color.Load(pt.get_child("color"));
}