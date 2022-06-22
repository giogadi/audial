#include "rigid_body.h"

#include "imgui/imgui.h"

#include "collisions.h"

bool RigidBodyComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    _collisionMgr = g._collisionManager;
    _collisionMgr->AddBody(e.FindComponentOfType<RigidBodyComponent>());
    return !_transform.expired();
}

bool RigidBodyComponent::DrawImGui() {
    ImGui::Checkbox("Static##", &_static);

    // TODO: maybe only update the AABB if the value is changed
    ImGui::InputScalar("MinX##", ImGuiDataType_Float, &_localAabb._min._x, /*step=*/nullptr, /*???*/NULL, "%f");
    ImGui::InputScalar("MinY##", ImGuiDataType_Float, &_localAabb._min._y, /*step=*/nullptr, /*???*/NULL, "%f");
    ImGui::InputScalar("MinZ##", ImGuiDataType_Float, &_localAabb._min._z, /*step=*/nullptr, /*???*/NULL, "%f");

    ImGui::InputScalar("MaxX##", ImGuiDataType_Float, &_localAabb._max._x, /*step=*/nullptr, /*???*/NULL, "%f");
    ImGui::InputScalar("MaxY##", ImGuiDataType_Float, &_localAabb._max._y, /*step=*/nullptr, /*???*/NULL, "%f");
    ImGui::InputScalar("MaxZ##", ImGuiDataType_Float, &_localAabb._max._z, /*step=*/nullptr, /*???*/NULL, "%f");

    return false;
}