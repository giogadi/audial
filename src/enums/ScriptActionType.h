#pragma once



enum class ScriptActionType : int {
    
    DestroyAllPlanets,
    
    ActivateEntity,
    
    AudioEvent,
    
    StartWaypointFollow,
    
    Count
};
extern char const* gScriptActionTypeStrings[];
char const* ScriptActionTypeToString(ScriptActionType e);
ScriptActionType StringToScriptActionType(char const* s);

bool ScriptActionTypeImGui(char const* label, ScriptActionType* v);

