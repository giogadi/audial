#include "src/enums/StepSeqParamType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, StepSeqParamType> const gStringToStepSeqParamType = {
    
    { "Velocities", StepSeqParamType::Velocities },
    
    { "NoteLength", StepSeqParamType::NoteLength }
    
};

} // end namespace

char const* gStepSeqParamTypeStrings[] = {
	
    "Velocities",
    
    "NoteLength"
    
};

char const* StepSeqParamTypeToString(StepSeqParamType e) {
	return gStepSeqParamTypeStrings[static_cast<int>(e)];
}

StepSeqParamType StringToStepSeqParamType(char const* s) {
    auto iter = gStringToStepSeqParamType.find(s);
    if (iter != gStringToStepSeqParamType.end()) {
    	return gStringToStepSeqParamType.at(s);
    }
    printf("ERROR StringToStepSeqParamType: unrecognized value \"%s\"\n", s);
    return static_cast<StepSeqParamType>(0);
}

bool StepSeqParamTypeImGui(char const* label, StepSeqParamType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gStepSeqParamTypeStrings, static_cast<int>(StepSeqParamType::Count));
    if (changed) {
        *v = static_cast<StepSeqParamType>(selectedIx);
    }
    return changed;
}


char const* EnumToString(StepSeqParamType e) {
     return StepSeqParamTypeToString(e);
}

void StringToEnum(char const* s, StepSeqParamType& e) {
     e = StringToStepSeqParamType(s);
}