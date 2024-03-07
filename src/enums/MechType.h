#pragma once



enum class MechType : int {
    
    Spawner,
    
    Grabber,
    
    Sink,
    
    Count
};
extern char const* gMechTypeStrings[];
char const* MechTypeToString(MechType e);
MechType StringToMechType(char const* s);

bool MechTypeImGui(char const* label, MechType* v);


char const* EnumToString(MechType e);
void StringToEnum(char const* s, MechType& e);