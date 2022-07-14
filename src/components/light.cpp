#include "light.h"

#include "imgui/imgui.h"

#include "serial.h"
#include "entity.h"
#include "game_manager.h"
#include "components/transform.h"
#include "renderer.h"

bool LightComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    _mgr = g._sceneManager;
    _mgr->AddLight(e.FindComponentOfType<LightComponent>());
    return !_transform.expired();
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