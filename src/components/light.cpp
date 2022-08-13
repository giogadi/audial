#include "light.h"

#include "imgui/imgui.h"

#include "serial.h"
#include "entity.h"
#include "game_manager.h"
#include "components/transform.h"
#include "renderer.h"

bool LightComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    _scene = g._scene;
    std::pair<VersionId, renderer::PointLight*> item = _scene->AddPointLight();
    _lightId = item.first;
    renderer::PointLight* l = item.second;
    Vec3 p = _transform.lock()->GetWorldPos();
    l->Set(p, _ambient, _diffuse);    
    return !_transform.expired();
}

void LightComponent::Update(float dt) {
    // TODO: maybe use a dirty flag so we only do this if this light has been changed.
    renderer::PointLight* l = _scene->GetPointLight(_lightId);
    Vec3 p = _transform.lock()->GetWorldPos();
    l->Set(p, _ambient, _diffuse);
}

void LightComponent::EditModeUpdate(float dt) {
    Update(dt);
}

void LightComponent::Destroy() {
    if (_scene) {
        _scene->RemovePointLight(_lightId);
    }    
}

void LightComponent::EditDestroy() {
    Destroy();
}

bool LightComponent::DrawImGui() {
    ImGui::InputScalar("Diffuse R", ImGuiDataType_Float, &_diffuse._x, /*step=*/nullptr);
    ImGui::InputScalar("Diffuse G", ImGuiDataType_Float, &_diffuse._y, /*step=*/nullptr);
    ImGui::InputScalar("Diffuse B", ImGuiDataType_Float, &_diffuse._z, /*step=*/nullptr);

    ImGui::InputScalar("Ambient R", ImGuiDataType_Float, &_ambient._x, /*step=*/nullptr);
    ImGui::InputScalar("Ambient G", ImGuiDataType_Float, &_ambient._y, /*step=*/nullptr);
    ImGui::InputScalar("Ambient B", ImGuiDataType_Float, &_ambient._z, /*step=*/nullptr);

    return false;
}

void LightComponent::Save(ptree& pt) const {
    serial::SaveInNewChildOf(pt, "ambient", _ambient);
    serial::SaveInNewChildOf(pt, "diffuse", _diffuse);
}
void LightComponent::Load(ptree const& pt) {
    _ambient.Load(pt.get_child("ambient"));
    _diffuse.Load(pt.get_child("diffuse"));
}