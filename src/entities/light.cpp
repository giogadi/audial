#include "entities/light.h"

#include "game_manager.h"
#include "renderer.h"
#include "imgui/imgui.h"
#include "serial.h"

void LightEntity::DebugPrint() {
    ne::Entity::DebugPrint();
    printf("ambient: %f %f %f\n", _ambient._x, _ambient._y, _ambient._z);
    printf("diffuse: %f %f %f\n", _diffuse._x, _diffuse._y, _diffuse._z);
}
void LightEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "ambient", _ambient);
    serial::SaveInNewChildOf(pt, "diffuse", _diffuse);
}
void LightEntity::LoadDerived(serial::Ptree pt) {
    _ambient.Load(pt.GetChild("ambient"));
    _diffuse.Load(pt.GetChild("diffuse"));
}
void LightEntity::ImGuiDerived() {
    ImGui::ColorEdit3("Diffuse", _diffuse._data);
    ImGui::ColorEdit3("Ambient", _ambient._data);
}
void LightEntity::Init(GameManager& g) {
    _lightId = g._scene->AddPointLight().first;
}
void LightEntity::Update(GameManager& g, float dt) {
    renderer::PointLight* light = g._scene->GetPointLight(_lightId);
    light->Set(_transform.GetPos(), _ambient, _diffuse);
}
void LightEntity::Destroy(GameManager& g) {
    g._scene->RemovePointLight(_lightId);
}