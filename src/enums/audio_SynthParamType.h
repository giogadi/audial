#pragma once


namespace audio {


enum class SynthParamType : int {
    
    Gain,
    
    Cutoff,
    
    Peak,
    
    PitchLFOGain,
    
    PitchLFOFreq,
    
    CutoffLFOGain,
    
    CutoffLFOFreq,
    
    AmpEnvAttack,
    
    AmpEnvDecay,
    
    AmpEnvSustain,
    
    AmpEnvRelease,
    
    CutoffEnvGain,
    
    CutoffEnvAttack,
    
    CutoffEnvDecay,
    
    CutoffEnvSustain,
    
    CutoffEnvRelease,
    
    Count
};
char const* SynthParamTypeToString(SynthParamType e);
SynthParamType StringToSynthParamType(char const* s);


}