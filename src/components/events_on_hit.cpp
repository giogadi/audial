#include "events_on_hit.h"

#include <sstream>

#include "imgui/imgui.h"

#include "beat_clock.h"
#include "audio.h"
#include "rigid_body.h"
#include "audio_event_imgui.h"
#include "components/transform.h"
#include "entity.h"
#include "game_manager.h"
#include "script_action.h"

EventsOnHitComponent::EventsOnHitComponent() {}

bool EventsOnHitComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _g = &g;
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

void EventsOnHitComponent::Save(ptree& pt) const {
    ScriptAction::SaveActions(pt.add_child("script_actions", ptree()), _actions);
}

void EventsOnHitComponent::Load(ptree const& pt) {
    ScriptAction::LoadActions(pt.get_child("script_actions"), _actions);
}