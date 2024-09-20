#include "src/enums/AutomationType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, AutomationType> const gStringToAutomationType = {
    
    { "SynthPatch", AutomationType::SynthPatch },
    
    { "Bpm", AutomationType::Bpm },
    
    { "StepSeqGain", AutomationType::StepSeqGain }
    
};

} // end namespace

char const* gAutomationTypeStrings[] = {
	
    "SynthPatch",
    
    "Bpm",
    
    "StepSeqGain"
    
};

char const* AutomationTypeToString(AutomationType e) {
	return gAutomationTypeStrings[static_cast<int>(e)];
}

AutomationType StringToAutomationType(char const* s) {
    auto iter = gStringToAutomationType.find(s);
    if (iter != gStringToAutomationType.end()) {
    	return gStringToAutomationType.at(s);
    }
    printf("ERROR StringToAutomationType: unrecognized value \"%s\"\n", s);
    return static_cast<AutomationType>(0);
}

bool AutomationTypeImGui(char const* label, AutomationType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gAutomationTypeStrings, static_cast<int>(AutomationType::Count));
    if (changed) {
        *v = static_cast<AutomationType>(selectedIx);
    }
    return changed;
}


char const* EnumToString(AutomationType e) {
     return AutomationTypeToString(e);
}

void StringToEnum(char const* s, AutomationType& e) {
     e = StringToAutomationType(s);
}