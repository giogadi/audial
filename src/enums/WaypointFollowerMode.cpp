#include "src/enums/WaypointFollowerMode.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, WaypointFollowerMode> const gStringToWaypointFollowerMode = {
    
    { "Waypoints", WaypointFollowerMode::Waypoints },
    
    { "Random", WaypointFollowerMode::Random }
    
};

} // end namespace

char const* gWaypointFollowerModeStrings[] = {
	
    "Waypoints",
    
    "Random"
    
};

char const* WaypointFollowerModeToString(WaypointFollowerMode e) {
	return gWaypointFollowerModeStrings[static_cast<int>(e)];
}

WaypointFollowerMode StringToWaypointFollowerMode(char const* s) {
    auto iter = gStringToWaypointFollowerMode.find(s);
    if (iter != gStringToWaypointFollowerMode.end()) {
    	return gStringToWaypointFollowerMode.at(s);
    }
    printf("ERROR StringToWaypointFollowerMode: unrecognized value \"%s\"\n", s);
    return static_cast<WaypointFollowerMode>(0);
}

char const* EnumToString(WaypointFollowerMode e) {
     return WaypointFollowerModeToString(e);
}

void StringToEnum(char const* s, WaypointFollowerMode& e) {
     e = StringToWaypointFollowerMode(s);
}

bool WaypointFollowerModeImGui(char const* label, WaypointFollowerMode* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gWaypointFollowerModeStrings, static_cast<int>(WaypointFollowerMode::Count));
    if (changed) {
        *v = static_cast<WaypointFollowerMode>(selectedIx);
    }
    return changed;
}

