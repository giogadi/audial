#include "src/enums/ScriptActionType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, ScriptActionType> const gStringToScriptActionType = {
    
    { "DestroyAllPlanets", ScriptActionType::DestroyAllPlanets },
    
    { "ActivateEntity", ScriptActionType::ActivateEntity },
    
    { "AudioEvent", ScriptActionType::AudioEvent },
    
    { "StartWaypointFollow", ScriptActionType::StartWaypointFollow }
    
};

} // end namespace

char const* gScriptActionTypeStrings[] = {
	
    "DestroyAllPlanets",
    
    "ActivateEntity",
    
    "AudioEvent",
    
    "StartWaypointFollow"
    
};

char const* ScriptActionTypeToString(ScriptActionType e) {
	return gScriptActionTypeStrings[static_cast<int>(e)];
}

ScriptActionType StringToScriptActionType(char const* s) {
    auto iter = gStringToScriptActionType.find(s);
    if (iter != gStringToScriptActionType.end()) {
    	return gStringToScriptActionType.at(s);
    }
    printf("ERROR StringToScriptActionType: unrecognized value \"%s\"\n", s);
    return static_cast<ScriptActionType>(0);
}

bool ScriptActionTypeImGui(char const* label, ScriptActionType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gScriptActionTypeStrings, static_cast<int>(ScriptActionType::Count));
    if (changed) {
        *v = static_cast<ScriptActionType>(selectedIx);
    }
    return changed;
}

