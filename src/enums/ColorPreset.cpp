#include "src/enums/ColorPreset.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, ColorPreset> const gStringToColorPreset = {
    
    { "White", ColorPreset::White },
    
    { "Black", ColorPreset::Black },
    
    { "Red", ColorPreset::Red },
    
    { "Orange", ColorPreset::Orange },
    
    { "Yellow", ColorPreset::Yellow },
    
    { "Green", ColorPreset::Green },
    
    { "Blue", ColorPreset::Blue },
    
    { "Magenta", ColorPreset::Magenta },
    
    { "Cyan", ColorPreset::Cyan },
    
    { "Gray", ColorPreset::Gray }
    
};

} // end namespace

char const* gColorPresetStrings[] = {
	
    "White",
    
    "Black",
    
    "Red",
    
    "Orange",
    
    "Yellow",
    
    "Green",
    
    "Blue",
    
    "Magenta",
    
    "Cyan",
    
    "Gray"
    
};

char const* ColorPresetToString(ColorPreset e) {
	return gColorPresetStrings[static_cast<int>(e)];
}

ColorPreset StringToColorPreset(char const* s) {
    auto iter = gStringToColorPreset.find(s);
    if (iter != gStringToColorPreset.end()) {
    	return gStringToColorPreset.at(s);
    }
    printf("ERROR StringToColorPreset: unrecognized value \"%s\"\n", s);
    return static_cast<ColorPreset>(0);
}

bool ColorPresetImGui(char const* label, ColorPreset* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gColorPresetStrings, static_cast<int>(ColorPreset::Count));
    if (changed) {
        *v = static_cast<ColorPreset>(selectedIx);
    }
    return changed;
}


char const* EnumToString(ColorPreset e) {
     return ColorPresetToString(e);
}

void StringToEnum(char const* s, ColorPreset& e) {
     e = StringToColorPreset(s);
}