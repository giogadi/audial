#include "components/model_instance.h"

#include "imgui/imgui.h"

#include "mesh.h"
#include "renderer.h"
#include "serial.h"
#include "entity.h"
#include "game_manager.h"
#include "components/transform.h"

bool ModelComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    if (_transform.expired()) {
        return false;
    }
    _scene = g._scene;
    _mesh = g._scene->GetMesh(_meshId);
    if (_mesh == nullptr) {
        return false;
    }
    return true;
}

void ModelComponent::Destroy() {
}

void ModelComponent::EditDestroy() {
    Destroy();
}

void ModelComponent::Update(float dt) {
    _scene->DrawMesh(_mesh, _transform.lock()->GetWorldMat4(), _color);
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

void ModelComponent::Save(serial::Ptree pt) const {
    pt.PutString("mesh_id", _meshId.c_str());
    serial::SaveInNewChildOf(pt, "color", _color);
}

void ModelComponent::Load(serial::Ptree pt) {
    if (!pt.TryGetString("mesh_id", &_meshId)) {
        // old version used "model_id"
        _meshId = pt.GetString("model_id");
    }
    _color.Load(pt.GetChild("color"));
}