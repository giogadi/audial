#include "src/enums/audio_SynthParamType.h"

#include <unordered_map>
#include <string>


namespace audio {


namespace {
char const* gSynthParamTypeStrings[] = {
	
    "Gain",
    
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

std::unordered_map<std::string, SynthParamType> const gStringToSynthParamType = {
    
    { "Gain", SynthParamType::Gain },
    
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

char const* SynthParamTypeToString(SynthParamType e) {
	return gSynthParamTypeStrings[static_cast<int>(e)];
}

SynthParamType StringToSynthParamType(char const* s) {
	return gStringToSynthParamType.at(s);
}


}
