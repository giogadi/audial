#include "script_action.h"

#include "imgui/imgui.h"

std::unique_ptr<ScriptAction> MakeScriptActionOfType(ScriptActionType actionType) {
    switch (actionType) {
        case ScriptActionType::DestroyAllPlanets: {
            return std::make_unique<ScriptActionDestroyAllPlanets>();
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
        sprintf(headerName, "%s##Header", ScriptActionTypeToString(actions[i]->Type()));
        if (ImGui::CollapsingHeader(headerName)) {
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