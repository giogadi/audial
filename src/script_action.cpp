#include "script_action.h"

#include "imgui/imgui.h"

#include "audio_event_imgui.h"
#include "serial.h"
#include "audio.h"
#include "game_manager.h"
#include "entity_manager.h"
#include "components/waypoint_follow.h"

std::unique_ptr<ScriptAction> MakeScriptActionOfType(ScriptActionType actionType) {
    switch (actionType) {
        case ScriptActionType::DestroyAllPlanets: {
            return std::make_unique<ScriptActionDestroyAllPlanets>();
            break;
        }
        case ScriptActionType::ActivateEntity: {
            return std::make_unique<ScriptActionActivateEntity>();
            break;
        }
        case ScriptActionType::AudioEvent: {
            return std::make_unique<ScriptActionAudioEvent>();
            break;
        }
        case ScriptActionType::StartWaypointFollow: {
            return std::make_unique<ScriptActionStartWaypointFollow>();
            break;
        }
        case ScriptActionType::Count: {
            assert(false);
            break;
        }
    }
}

void DrawScriptActionListImGui(std::vector<std::unique_ptr<ScriptAction>>& actions) {
    if (ImGui::Button("Add Action##")) {
        actions.emplace_back(MakeScriptActionOfType(ScriptActionType::DestroyAllPlanets));
    }

    char headerName[128];
    for (int i = 0; i < actions.size(); ++i) {
        ImGui::PushID(i);
        sprintf(headerName, "%s###Header", ScriptActionTypeToString(actions[i]->Type()));
        if (ImGui::CollapsingHeader(headerName)) {
            if (ImGui::Button("Delete Action")) {
                actions.erase(actions.begin() + i);
                --i;
                ImGui::PopID();
                continue;
            }
            int selectedActionTypeIx = static_cast<int>(actions[i]->Type());
            bool changed = ImGui::Combo(
                "Type##", &selectedActionTypeIx, gScriptActionTypeStrings,
                static_cast<int>(ScriptActionType::Count));
            if (changed && selectedActionTypeIx >= 0) {
                // When type changes, we have to rebuild the action.
                ScriptActionType newType = static_cast<ScriptActionType>(selectedActionTypeIx);
                actions[i] = MakeScriptActionOfType(newType);
            }
            actions[i]->DrawImGui();
        }
        ImGui::PopID();
    }
}

void ScriptAction::SaveActions(ptree& pt, std::vector<std::unique_ptr<ScriptAction>> const& actions)  {
    for (auto const& action : actions) {
        ptree actionPt;
        actionPt.put("script_action_type", ScriptActionTypeToString(action->Type()));
        action->Save(actionPt);
        pt.add_child("script_action", actionPt);
    }
}

void ScriptAction::LoadActions(ptree const& pt, std::vector<std::unique_ptr<ScriptAction>>& actions) {
    for (auto const& item : pt) {
        ptree const& actionPt = item.second;
        ScriptActionType actionType =
            StringToScriptActionType(actionPt.get<std::string>("script_action_type").c_str());
        std::unique_ptr<ScriptAction> newAction = MakeScriptActionOfType(actionType);
        newAction->Load(actionPt);
        actions.push_back(std::move(newAction));
    }
}

void ScriptActionDestroyAllPlanets::Execute(GameManager& g) const {
    g._entityManager->ForEveryActiveEntity([&g](EntityId id) {
        Entity* entity = g._entityManager->GetEntity(id);
        bool hasPlanet = !entity->FindComponentOfType<OrbitableComponent>().expired();
        if (hasPlanet) {
            g._entityManager->TagEntityForDestroy(id);
        }
    });
}

void ScriptActionActivateEntity::Execute(GameManager& g) const {
    EntityId id = g._entityManager->FindInactiveEntityByName(_entityName.c_str());
    g._entityManager->ActivateEntity(id, g);
}

void ScriptActionActivateEntity::DrawImGui() {
    char name[128];
    strcpy(name, _entityName.c_str());
    if (ImGui::InputText("Name##", name, 128)) {
        _entityName = name;
    }
}

void ScriptActionActivateEntity::Save(ptree& pt) const {
    pt.put("entity_name", _entityName);
}

void ScriptActionActivateEntity::Load(ptree const& pt) {
    _entityName = pt.get<std::string>("entity_name");
}

namespace {
audio::Event GetEventAtBeatOffsetFromNextDenom(double denom, BeatTimeEvent const& b_e, BeatClock const& beatClock) {
    double beatTime = beatClock.GetBeatTime();
    double startTime = BeatClock::GetNextBeatDenomTime(beatTime, denom);
    unsigned long startTickTime = beatClock.BeatTimeToTickTime(startTime);
    audio::Event e = b_e._e;
    e.timeInTicks = beatClock.BeatTimeToTickTime(b_e._beatTime) + startTickTime;
    return e;
}
}

void ScriptActionAudioEvent::Execute(GameManager& g) const {
    audio::Event e = GetEventAtBeatOffsetFromNextDenom(_denom, _event, *g._beatClock);
    g._audioContext->AddEvent(e);
}

void ScriptActionAudioEvent::DrawImGui() {
    ImGui::InputScalar("Denom##", ImGuiDataType_Double, &_denom);
    ImGui::InputScalar("Beat time##", ImGuiDataType_Double, &_event._beatTime);
    audio::EventDrawImGuiNoTime(_event._e);
}

void ScriptActionAudioEvent::Save(ptree& pt) const {
    pt.put("denom", _denom);
    serial::SaveInNewChildOf(pt, "beat_event", _event);
}

void ScriptActionAudioEvent::Load(ptree const& pt) {
    _denom = pt.get<double>("denom");
    _event.Load(pt.get_child("beat_event"));
}

void ScriptActionStartWaypointFollow::Execute(GameManager& g) const {
    EntityId id = g._entityManager->FindActiveEntityByName(_entityName.c_str());
    if (!id.IsValid()) {
        printf("ScriptActionStartWaypointFollow: entity not found: %s\n", _entityName.c_str());
        return;
    }
    Entity* e = g._entityManager->GetEntity(id);
    assert(e != nullptr);

    std::shared_ptr<WaypointFollowComponent> comp = e->FindComponentOfType<WaypointFollowComponent>().lock();
    if (comp == nullptr) {
        printf("ScriptActionStartWaypointFollow: entity \"%s\" has no WaypointFollowComponent.\n", _entityName.c_str());
        return;
    }

    comp->_running = true;
}

void ScriptActionStartWaypointFollow::Save(ptree& pt) const {
    pt.put("entity_name", _entityName);
}

void ScriptActionStartWaypointFollow::Load(ptree const& pt) {
    _entityName = pt.get<std::string>("entity_name");
}

void ScriptActionStartWaypointFollow::DrawImGui() {
    char name[128];
    strcpy(name, _entityName.c_str());
    if (ImGui::InputText("Name##", name, 128)) {
        _entityName = name;
    }
}