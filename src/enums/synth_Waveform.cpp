#include "src/enums/synth_Waveform.h"

#include <unordered_map>
#include <string>
#include <cstdio>


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


}
