#pragma once



enum class ColorPreset : int {
    
    White,
    
    Black,
    
    Red,
    
    Orange,
    
    Yellow,
    
    Green,
    
    Blue,
    
    Magenta,
    
    Cyan,
    
    Count
};
extern char const* gColorPresetStrings[];
char const* ColorPresetToString(ColorPreset e);
ColorPreset StringToColorPreset(char const* s);

