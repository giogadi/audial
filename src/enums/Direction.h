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

