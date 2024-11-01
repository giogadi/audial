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

extern GameManager gGameManager;

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

bool DrawScriptActionListImGui(std::vector<std::unique_ptr<ScriptAction>>& actions) {
    bool shouldReInit = false;

    if (ImGui::Button("Add Action##")) {
        actions.emplace_back(MakeScriptActionOfType(ScriptActionType::DestroyAllPlanets));
        shouldReInit = true;
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
                shouldReInit = true;
            }
            bool thisActionNeedsReInit = actions[i]->DrawImGui();
            shouldReInit = shouldReInit || thisActionNeedsReInit;
        }
        ImGui::PopID();
    }

    return shouldReInit;
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

void ScriptActionActivateEntity::InitImpl(EntityId /*entityId*/, GameManager& g) {
    _entityId = g._entityManager->FindInactiveEntityByName(_entityName.c_str());
}

void ScriptActionActivateEntity::ExecuteImpl(GameManager& g) const {
    g._entityManager->ActivateEntity(_entityId, g);
}

bool ScriptActionActivateEntity::DrawImGui() {
    char name[128];
    strcpy(name, _entityName.c_str());
    if (ImGui::InputText("Name##", name, 128)) {
        _entityName = name;
    }
    return false;
}

void ScriptActionActivateEntity::Save(serial::Ptree pt) const {
    pt.PutString("entity_name", _entityName.c_str());
}

void ScriptActionActivateEntity::Load(serial::Ptree pt) {
    _entityName = pt.GetString("entity_name");
}

void ScriptActionAudioEvent::InitImpl(EntityId entityId, GameManager& g) {
    // Look for an orbitable component on this entity, and see if it has the recorder name populated.
    if (Entity* entity = g._entityManager->GetEntity(entityId)) {
        std::shared_ptr<OrbitableComponent> orbitable = entity->FindComponentOfType<OrbitableComponent>().lock();
        if (orbitable != nullptr && !orbitable->_recorderName.empty()) {
            _recorderId = g._entityManager->FindActiveOrInactiveEntityByName(orbitable->_recorderName.c_str());
            if (!_recorderId.IsValid()) {
                printf("Couldn't find recorder of name \"%s\"\n.", orbitable->_recorderName.c_str());
            }
        }
    }
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

bool ScriptActionAudioEvent::DrawImGui() {
    ImGui::InputScalar("Denom##", ImGuiDataType_Double, &_denom);
    ImGui::InputScalar("Beat time##", ImGuiDataType_Double, &_event._beatTime);
    audio::EventDrawImGuiNoTime(_event._e, *gGameManager._soundBank);
    return false;
}

void ScriptActionAudioEvent::Save(serial::Ptree pt) const {
    pt.PutDouble("denom", _denom);
    serial::SaveInNewChildOf(pt, "beat_event", _event);
}

void ScriptActionAudioEvent::Load(serial::Ptree pt) {
    _denom = pt.GetDouble("denom");
    _event.Load(pt.GetChild("beat_event"));
}

void ScriptActionStartWaypointFollow::InitImpl(EntityId /*entityId*/, GameManager& g) {
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

bool ScriptActionStartWaypointFollow::DrawImGui() {
    char name[128];
    strcpy(name, _entityName.c_str());
    if (ImGui::InputText("Name##", name, 128)) {
        _entityName = name;
        return true;
    }
    return false;
}