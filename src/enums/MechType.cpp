#include "src/enums/MechType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, MechType> const gStringToMechType = {
    
    { "Spawner", MechType::Spawner },
    
    { "Pusher", MechType::Pusher },
    
    { "Sink", MechType::Sink }
    
};

} // end namespace

char const* gMechTypeStrings[] = {
	
    "Spawner",
    
    "Pusher",
    
    "Sink"
    
};

char const* MechTypeToString(MechType e) {
	return gMechTypeStrings[static_cast<int>(e)];
}

MechType StringToMechType(char const* s) {
    auto iter = gStringToMechType.find(s);
    if (iter != gStringToMechType.end()) {
    	return gStringToMechType.at(s);
    }
    printf("ERROR StringToMechType: unrecognized value \"%s\"\n", s);
    return static_cast<MechType>(0);
}

bool MechTypeImGui(char const* label, MechType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gMechTypeStrings, static_cast<int>(MechType::Count));
    if (changed) {
        *v = static_cast<MechType>(selectedIx);
    }
    return changed;
}


char const* EnumToString(MechType e) {
     return MechTypeToString(e);
}

void StringToEnum(char const* s, MechType& e) {
     e = StringToMechType(s);
}