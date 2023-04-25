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
    
    Gray,
    
    Count
};
extern char const* gColorPresetStrings[];
char const* ColorPresetToString(ColorPreset e);
ColorPreset StringToColorPreset(char const* s);

bool ColorPresetImGui(char const* label, ColorPreset* v);


char const* EnumToString(ColorPreset e);
void StringToEnum(char const* s, ColorPreset& e);