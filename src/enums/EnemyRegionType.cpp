#include "src/enums/EnemyRegionType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, EnemyRegionType> const gStringToEnemyRegionType = {
    
    { "None", EnemyRegionType::None },
    
    { "TriggerEntity", EnemyRegionType::TriggerEntity },
    
    { "BoundedXAxis", EnemyRegionType::BoundedXAxis }
    
};

} // end namespace

char const* gEnemyRegionTypeStrings[] = {
	
    "None",
    
    "TriggerEntity",
    
    "BoundedXAxis"
    
};

char const* EnemyRegionTypeToString(EnemyRegionType e) {
	return gEnemyRegionTypeStrings[static_cast<int>(e)];
}

EnemyRegionType StringToEnemyRegionType(char const* s) {
    auto iter = gStringToEnemyRegionType.find(s);
    if (iter != gStringToEnemyRegionType.end()) {
    	return gStringToEnemyRegionType.at(s);
    }
    printf("ERROR StringToEnemyRegionType: unrecognized value \"%s\"\n", s);
    return static_cast<EnemyRegionType>(0);
}

bool EnemyRegionTypeImGui(char const* label, EnemyRegionType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gEnemyRegionTypeStrings, static_cast<int>(EnemyRegionType::Count));
    if (changed) {
        *v = static_cast<EnemyRegionType>(selectedIx);
    }
    return changed;
}


char const* EnumToString(EnemyRegionType e) {
     return EnemyRegionTypeToString(e);
}

void StringToEnum(char const* s, EnemyRegionType& e) {
     e = StringToEnemyRegionType(s);
}