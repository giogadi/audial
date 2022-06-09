#pragma once



enum class ScriptActionType : int {
    
    DestroyAllPlanets,
    
    ActivateEntity,
    
    Count
};
extern char const* gScriptActionTypeStrings[];
char const* ScriptActionTypeToString(ScriptActionType e);
ScriptActionType StringToScriptActionType(char const* s);

