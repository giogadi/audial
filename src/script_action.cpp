#include "script_action.h"

#include "imgui/imgui.h"

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