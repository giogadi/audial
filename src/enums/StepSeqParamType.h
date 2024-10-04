#pragma once



enum class StepSeqParamType : int {
    
    Velocities,
    
    NoteLength,
    
    Gain,
    
    Count
};
extern char const* gStepSeqParamTypeStrings[];
char const* StepSeqParamTypeToString(StepSeqParamType e);
StepSeqParamType StringToStepSeqParamType(char const* s);

bool StepSeqParamTypeImGui(char const* label, StepSeqParamType* v);


char const* EnumToString(StepSeqParamType e);
void StringToEnum(char const* s, StepSeqParamType& e);