#include "src/enums/ScriptActionType.h"

#include <unordered_map>
#include <string>



namespace {

std::unordered_map<std::string, ScriptActionType> const gStringToScriptActionType = {
    
    { "DestroyAllPlanets", ScriptActionType::DestroyAllPlanets },
    
    { "ActivateEntity", ScriptActionType::ActivateEntity }
    
};

} // end namespace

char const* gScriptActionTypeStrings[] = {
	
    "DestroyAllPlanets",
    
    "ActivateEntity"
    
};

char const* ScriptActionTypeToString(ScriptActionType e) {
	return gScriptActionTypeStrings[static_cast<int>(e)];
}

ScriptActionType StringToScriptActionType(char const* s) {
	return gStringToScriptActionType.at(s);
}

