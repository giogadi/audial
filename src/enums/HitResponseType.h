#pragma once



enum class HitResponseType : int {
    
    None,
    
    Pull,
    
    Push,
    
    Count
};
extern char const* gHitResponseTypeStrings[];
char const* HitResponseTypeToString(HitResponseType e);
HitResponseType StringToHitResponseType(char const* s);

bool HitResponseTypeImGui(char const* label, HitResponseType* v);


char const* EnumToString(HitResponseType e);
void StringToEnum(char const* s, HitResponseType& e);