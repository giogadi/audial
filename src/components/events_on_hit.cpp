#include "events_on_hit.h"

#include "imgui/imgui.h"

#include "beat_clock.h"
#include "entity.h"
#include "game_manager.h"
#include "script_action.h"

EventsOnHitComponent::EventsOnHitComponent() {}

bool EventsOnHitComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _g = &g;
    for (auto& action : _actions) {
        action->Init(id, g);
    }
    return true;
}

void EventsOnHitComponent::AddOnHitCallback(OnHitCallback callback) {
    _callbacks.push_back(std::move(callback));
}

void EventsOnHitComponent::OnHit(EntityId other) const {
    for (auto const& action : _actions) {
        action->Execute(*_g);
    }
    for (auto const& callback : _callbacks) {
        callback(other);
    }
}

bool EventsOnHitComponent::DrawImGui() {
    ImGui::PushID(static_cast<int>(Type()));
    DrawScriptActionListImGui(_actions);
    ImGui::PopID();
    return false;  // no reconnect
}

void EventsOnHitComponent::OnEditPick() {
    for (auto const& action : _actions) {
        action->Execute(*_g);
    }
    // TODO: add option to maybe call callbacks on edit pick?
    // for (auto const& callback : _callbacks) {
    //     callback(other);
    // }
}

void EventsOnHitComponent::Save(serial::Ptree pt) const {
    serial::Ptree actionsPt = pt.AddChild("script_actions");
    ScriptAction::SaveActions(actionsPt, _actions);    
}

void EventsOnHitComponent::Load(serial::Ptree pt) {
    ScriptAction::LoadActions(pt.GetChild("script_actions"), _actions);
}