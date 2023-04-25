#include "src/enums/Direction.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, Direction> const gStringToDirection = {
    
    { "Center", Direction::Center },
    
    { "Up", Direction::Up },
    
    { "Down", Direction::Down },
    
    { "Left", Direction::Left },
    
    { "Right", Direction::Right }
    
};

} // end namespace

char const* gDirectionStrings[] = {
	
    "Center",
    
    "Up",
    
    "Down",
    
    "Left",
    
    "Right"
    
};

char const* DirectionToString(Direction e) {
	return gDirectionStrings[static_cast<int>(e)];
}

Direction StringToDirection(char const* s) {
    auto iter = gStringToDirection.find(s);
    if (iter != gStringToDirection.end()) {
    	return gStringToDirection.at(s);
    }
    printf("ERROR StringToDirection: unrecognized value \"%s\"\n", s);
    return static_cast<Direction>(0);
}

bool DirectionImGui(char const* label, Direction* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gDirectionStrings, static_cast<int>(Direction::Count));
    if (changed) {
        *v = static_cast<Direction>(selectedIx);
    }
    return changed;
}


char const* EnumToString(Direction e) {
     return DirectionToString(e);
}

void StringToEnum(char const* s, Direction& e) {
     e = StringToDirection(s);
}