#pragma once



enum class Direction : int {
    
    Center,
    
    Up,
    
    Down,
    
    Left,
    
    Right,
    
    Count
};
extern char const* gDirectionStrings[];
char const* DirectionToString(Direction e);
Direction StringToDirection(char const* s);
char const* EnumToString(Direction e);
void StringToEnum(char const* s, Direction& e);

bool DirectionImGui(char const* label, Direction* v);

