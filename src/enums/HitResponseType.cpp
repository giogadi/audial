#include "src/enums/HitResponseType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, HitResponseType> const gStringToHitResponseType = {
    
    { "None", HitResponseType::None },
    
    { "Pull", HitResponseType::Pull },
    
    { "Push", HitResponseType::Push }
    
};

} // end namespace

char const* gHitResponseTypeStrings[] = {
	
    "None",
    
    "Pull",
    
    "Push"
    
};

char const* HitResponseTypeToString(HitResponseType e) {
	return gHitResponseTypeStrings[static_cast<int>(e)];
}

HitResponseType StringToHitResponseType(char const* s) {
    auto iter = gStringToHitResponseType.find(s);
    if (iter != gStringToHitResponseType.end()) {
    	return gStringToHitResponseType.at(s);
    }
    printf("ERROR StringToHitResponseType: unrecognized value \"%s\"\n", s);
    return static_cast<HitResponseType>(0);
}

bool HitResponseTypeImGui(char const* label, HitResponseType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gHitResponseTypeStrings, static_cast<int>(HitResponseType::Count));
    if (changed) {
        *v = static_cast<HitResponseType>(selectedIx);
    }
    return changed;
}


char const* EnumToString(HitResponseType e) {
     return HitResponseTypeToString(e);
}

void StringToEnum(char const* s, HitResponseType& e) {
     e = StringToHitResponseType(s);
}