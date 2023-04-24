#pragma once



enum class WaypointFollowerMode : int {
    
    Waypoints,
    
    Random,
    
    Count
};
extern char const* gWaypointFollowerModeStrings[];
char const* WaypointFollowerModeToString(WaypointFollowerMode e);
WaypointFollowerMode StringToWaypointFollowerMode(char const* s);
char const* EnumToString(WaypointFollowerMode e);
void StringToEnum(char const* s, WaypointFollowerMode& e);

bool WaypointFollowerModeImGui(char const* label, WaypointFollowerMode* v);

