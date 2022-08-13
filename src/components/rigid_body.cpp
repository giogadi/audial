#include "rigid_body.h"

#include "imgui/imgui.h"

#include "collisions.h"
#include "transform.h"
#include "entity.h"
#include "game_manager.h"

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

void RigidBodyComponent::Save(serial::Ptree pt) const {
    pt.PutBool("static", _static);
    pt.PutString("layer", CollisionLayerToString(_layer));
    serial::SaveInNewChildOf(pt, "aabb", _localAabb);
}

void RigidBodyComponent::Load(serial::Ptree pt) {
    _static = pt.GetBool("static");
    _layer = StringToCollisionLayer(pt.GetString("layer").c_str());
    _localAabb.Load(pt.GetChild("aabb"));
}