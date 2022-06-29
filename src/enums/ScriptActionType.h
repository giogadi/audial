#pragma once



enum class ScriptActionType : int {
    
    DestroyAllPlanets,
    
    ActivateEntity,
    
    AudioEvent,
    
    Count
};
extern char const* gScriptActionTypeStrings[];
char const* ScriptActionTypeToString(ScriptActionType e);
ScriptActionType StringToScriptActionType(char const* s);

