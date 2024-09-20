#pragma once



enum class AutomationType : int {
    
    SynthPatch,
    
    Bpm,
    
    StepSeqGain,
    
    Count
};
extern char const* gAutomationTypeStrings[];
char const* AutomationTypeToString(AutomationType e);
AutomationType StringToAutomationType(char const* s);

bool AutomationTypeImGui(char const* label, AutomationType* v);


char const* EnumToString(AutomationType e);
void StringToEnum(char const* s, AutomationType& e);