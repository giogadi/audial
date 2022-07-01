#include "src/enums/synth_Waveform.h"

#include <unordered_map>
#include <string>


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
	return gStringToWaveform.at(s);
}


}
