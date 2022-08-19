#include "script_action.h"

#include "imgui/imgui.h"

#include "audio_event_imgui.h"
#include "serial.h"
#include "audio.h"
#include "game_manager.h"
#include "entity_manager.h"
#include "components/waypoint_follow.h"
#include "imgui_util.h"
#include "components/area_recorder.h"

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

void ScriptAction::SaveActions(serial::Ptree pt, std::vector<std::unique_ptr<ScriptAction>> const& actions) {
    for (auto const& action : actions) {
        serial::Ptree actionPt = pt.AddChild("script_action");
        actionPt.PutString("script_action_type", ScriptActionTypeToString(action->Type()));
        action->Save(actionPt);
    }
}

void ScriptAction::LoadActions(serial::Ptree pt, std::vector<std::unique_ptr<ScriptAction>>& actions) {
    int numChildren = 0;
    serial::NameTreePair* children = pt.GetChildren(&numChildren);
    for (int i = 0; i < numChildren; ++i) {
        serial::Ptree& actionPt = children[i]._pt;
        ScriptActionType actionType =
            StringToScriptActionType(actionPt.GetString("script_action_type").c_str());
        std::unique_ptr<ScriptAction> newAction = MakeScriptActionOfType(actionType);
        newAction->Load(actionPt);
        actions.push_back(std::move(newAction));
    }
    // free(children);
    delete[] children;
}

void ScriptActionDestroyAllPlanets::ExecuteImpl(GameManager& g) const {
    g._entityManager->ForEveryActiveEntity([&g](EntityId id) {
        Entity* entity = g._entityManager->GetEntity(id);
        bool hasPlanet = !entity->FindComponentOfType<OrbitableComponent>().expired();
        if (hasPlanet) {
            g._entityManager->TagEntityForDestroy(id);
        }
    });
}

void ScriptActionActivateEntity::InitImpl(GameManager& g) {
    _entityId = g._entityManager->FindInactiveEntityByName(_entityName.c_str());
}

void ScriptActionActivateEntity::ExecuteImpl(GameManager& g) const {
    g._entityManager->ActivateEntity(_entityId, g);
}

void ScriptActionActivateEntity::DrawImGui() {
    char name[128];
    strcpy(name, _entityName.c_str());
    if (ImGui::InputText("Name##", name, 128)) {
        _entityName = name;
    }
}

void ScriptActionActivateEntity::Save(serial::Ptree pt) const {
    pt.PutString("entity_name", _entityName.c_str());
}

void ScriptActionActivateEntity::Load(serial::Ptree pt) {
    _entityName = pt.GetString("entity_name");
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

void ScriptActionAudioEvent::InitImpl(GameManager& g) {
    _recorderId = g._entityManager->FindActiveOrInactiveEntityByName(_recorderName.c_str());
}

void ScriptActionAudioEvent::ExecuteImpl(GameManager& g) const {
    audio::Event e = GetEventAtBeatOffsetFromNextDenom(_denom, _event, *g._beatClock);
    g._audioContext->AddEvent(e);
    Entity* recorderEntity = g._entityManager->GetEntity(_recorderId);
    if (recorderEntity != nullptr) {
        std::shared_ptr<AreaRecorderComponent> recorder =
            recorderEntity->FindComponentOfType<AreaRecorderComponent>().lock();
        if (recorder != nullptr) {
            recorder->Record(e);
        }
    }
}

void ScriptActionAudioEvent::DrawImGui() {
    ImGui::InputScalar("Denom##", ImGuiDataType_Double, &_denom);
    ImGui::InputScalar("Beat time##", ImGuiDataType_Double, &_event._beatTime);
    imgui_util::InputText128("Recorder name##", &_recorderName);
    audio::EventDrawImGuiNoTime(_event._e);
}

void ScriptActionAudioEvent::Save(serial::Ptree pt) const {
    pt.PutDouble("denom", _denom);
    serial::SaveInNewChildOf(pt, "beat_event", _event);
}

void ScriptActionAudioEvent::Load(serial::Ptree pt) {
    _denom = pt.GetDouble("denom");
    _event.Load(pt.GetChild("beat_event"));
}

void ScriptActionStartWaypointFollow::InitImpl(GameManager& g) {
    _entityId = g._entityManager->FindActiveEntityByName(_entityName.c_str());
}

void ScriptActionStartWaypointFollow::ExecuteImpl(GameManager& g) const {
    if (!_entityId.IsValid()) {
        printf("ScriptActionStartWaypointFollow: entity not found: %s\n", _entityName.c_str());
        return;
    }
    Entity* e = g._entityManager->GetEntity(_entityId);
    assert(e != nullptr);

    std::shared_ptr<WaypointFollowComponent> comp = e->FindComponentOfType<WaypointFollowComponent>().lock();
    if (comp == nullptr) {
        printf("ScriptActionStartWaypointFollow: entity \"%s\" has no WaypointFollowComponent.\n", _entityName.c_str());
        return;
    }

    comp->_running = true;
}

void ScriptActionStartWaypointFollow::Save(serial::Ptree pt) const {
    pt.PutString("entity_name", _entityName.c_str());
}

void ScriptActionStartWaypointFollow::Load(serial::Ptree pt) {
    _entityName = pt.GetString("entity_name");
}

void ScriptActionStartWaypointFollow::DrawImGui() {
    char name[128];
    strcpy(name, _entityName.c_str());
    if (ImGui::InputText("Name##", name, 128)) {
        _entityName = name;
    }
}