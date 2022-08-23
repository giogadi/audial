#include "orbitable.h"

#include <iostream>

#include "imgui/imgui.h"

#include "rigid_body.h"
#include "script_action.h"
#include "entity.h"
#include "components/transform.h"
#include "components/damage.h"
#include "imgui_util.h"

bool OrbitableComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _t = e.FindComponentOfType<TransformComponent>();
    if (_t.expired()) {
        return false;
    }
    _damage = e.FindComponentOfType<DamageComponent>();
    // _rb = e.FindComponentOfType<RigidBodyComponent>();
    // if (_rb.expired()) {
    //     return false;
    // }
    // _rb.lock()->_layer = CollisionLayer::None;
    return true;
}

void OrbitableComponent::OnLeaveOrbit(GameManager& g) {
    for (auto const& action : _onLeaveActions) {
        action->Execute(g);
    }
}

void OrbitableComponent::Save(serial::Ptree pt) const {
    serial::Ptree actionsPt = pt.AddChild("on_leave_actions");
    ScriptAction::SaveActions(actionsPt, _onLeaveActions);
    pt.PutString("recorder_name", _recorderName.c_str());
}

void OrbitableComponent::Load(serial::Ptree pt) {
    serial::Ptree onLeaveActionsPt = pt.TryGetChild("on_leave_actions");
    if (onLeaveActionsPt.IsValid()) {
        ScriptAction::LoadActions(onLeaveActionsPt, _onLeaveActions);
    } else {
        printf("WARNING: OrbitableComponent missing on_leave_actions\n");
    }
    pt.TryGetString("recorder_name", &_recorderName);
}

bool OrbitableComponent::DrawImGui() {
    imgui_util::InputText128("Recorder name##", &_recorderName);
    DrawScriptActionListImGui(_onLeaveActions);
    return false;
}