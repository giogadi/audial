#include "orbitable.h"

#include "imgui/imgui.h"

#include "rigid_body.h"
#include "script_action.h"

bool OrbitableComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _t = e.FindComponentOfType<TransformComponent>();
    if (_t.expired()) {
        return false;
    }
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        return false;
    }
    _rb.lock()->_layer = CollisionLayer::None;
    return true;
}

void OrbitableComponent::OnLeaveOrbit(GameManager& g) {
    for (auto const& action : _onLeaveActions) {
        action->Execute(g);
    }
}

void OrbitableComponent::Save(ptree& pt) const {
    ScriptAction::SaveActions(pt.add_child("on_leave_actions", ptree()), _onLeaveActions);
}

void OrbitableComponent::Load(ptree const& pt) {
    boost::optional<ptree const&> onLeaveActionsPt = pt.get_child_optional("on_leave_actions");
    if (onLeaveActionsPt.has_value()) {
        ScriptAction::LoadActions(onLeaveActionsPt.value(), _onLeaveActions);
    } else {
        std::cout << "WARNING: OrbitableComponent missing on_leave_actions" << std::endl;
    }
}

bool OrbitableComponent::DrawImGui() {
    DrawScriptActionListImGui(_onLeaveActions);
    return false;
}