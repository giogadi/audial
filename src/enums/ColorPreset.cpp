#include "src/enums/ColorPreset.h"

#include <unordered_map>
#include <string>



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
    
    { "Cyan", ColorPreset::Cyan }
    
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
    
    "Cyan"
    
};

char const* ColorPresetToString(ColorPreset e) {
	return gColorPresetStrings[static_cast<int>(e)];
}

ColorPreset StringToColorPreset(char const* s) {
	return gStringToColorPreset.at(s);
}

