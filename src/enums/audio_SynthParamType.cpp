#include "src/enums/audio_SynthParamType.h"

#include <unordered_map>
#include <string>


namespace audio {


namespace {

std::unordered_map<std::string, SynthParamType> const gStringToSynthParamType = {
    
    { "Gain", SynthParamType::Gain },
    
    { "Osc1Waveform", SynthParamType::Osc1Waveform },
    
    { "Osc2Waveform", SynthParamType::Osc2Waveform },
    
    { "Detune", SynthParamType::Detune },
    
    { "OscFader", SynthParamType::OscFader },
    
    { "Cutoff", SynthParamType::Cutoff },
    
    { "Peak", SynthParamType::Peak },
    
    { "PitchLFOGain", SynthParamType::PitchLFOGain },
    
    { "PitchLFOFreq", SynthParamType::PitchLFOFreq },
    
    { "CutoffLFOGain", SynthParamType::CutoffLFOGain },
    
    { "CutoffLFOFreq", SynthParamType::CutoffLFOFreq },
    
    { "AmpEnvAttack", SynthParamType::AmpEnvAttack },
    
    { "AmpEnvDecay", SynthParamType::AmpEnvDecay },
    
    { "AmpEnvSustain", SynthParamType::AmpEnvSustain },
    
    { "AmpEnvRelease", SynthParamType::AmpEnvRelease },
    
    { "CutoffEnvGain", SynthParamType::CutoffEnvGain },
    
    { "CutoffEnvAttack", SynthParamType::CutoffEnvAttack },
    
    { "CutoffEnvDecay", SynthParamType::CutoffEnvDecay },
    
    { "CutoffEnvSustain", SynthParamType::CutoffEnvSustain },
    
    { "CutoffEnvRelease", SynthParamType::CutoffEnvRelease }
    
};

} // end namespace

char const* gSynthParamTypeStrings[] = {
	
    "Gain",
    
    "Osc1Waveform",
    
    "Osc2Waveform",
    
    "Detune",
    
    "OscFader",
    
    "Cutoff",
    
    "Peak",
    
    "PitchLFOGain",
    
    "PitchLFOFreq",
    
    "CutoffLFOGain",
    
    "CutoffLFOFreq",
    
    "AmpEnvAttack",
    
    "AmpEnvDecay",
    
    "AmpEnvSustain",
    
    "AmpEnvRelease",
    
    "CutoffEnvGain",
    
    "CutoffEnvAttack",
    
    "CutoffEnvDecay",
    
    "CutoffEnvSustain",
    
    "CutoffEnvRelease"
    
};

char const* SynthParamTypeToString(SynthParamType e) {
	return gSynthParamTypeStrings[static_cast<int>(e)];
}

SynthParamType StringToSynthParamType(char const* s) {
	return gStringToSynthParamType.at(s);
}


}
