#include "src/enums/synth_Waveform.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"


namespace synth {


namespace {

std::unordered_map<std::string, Waveform> const gStringToWaveform = {
    
    { "Square", Waveform::Square },
    
    { "Saw", Waveform::Saw }
    
};

} // end namespace

char const* gWaveformStrings[] = {
	
    "Square",
    
    "Saw"
    
};

char const* WaveformToString(Waveform e) {
	return gWaveformStrings[static_cast<int>(e)];
}

Waveform StringToWaveform(char const* s) {
    auto iter = gStringToWaveform.find(s);
    if (iter != gStringToWaveform.end()) {
    	return gStringToWaveform.at(s);
    }
    printf("ERROR StringToWaveform: unrecognized value \"%s\"\n", s);
    return static_cast<Waveform>(0);
}

char const* EnumToString(Waveform e) {
     return WaveformToString(e);
}

void StringToEnum(char const* s, Waveform& e) {
     e = StringToWaveform(s);
}

bool WaveformImGui(char const* label, Waveform* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gWaveformStrings, static_cast<int>(Waveform::Count));
    if (changed) {
        *v = static_cast<Waveform>(selectedIx);
    }
    return changed;
}


}
