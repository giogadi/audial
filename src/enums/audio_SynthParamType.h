#pragma once


namespace audio {


enum class SynthParamType : int {
    
    Gain,
    
    Osc1Waveform,
    
    Osc2Waveform,
    
    Detune,
    
    OscFader,
    
    Cutoff,
    
    Peak,
    
    HpfCutoff,
    
    HpfPeak,
    
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
    
    PitchEnvGain,
    
    PitchEnvAttack,
    
    PitchEnvDecay,
    
    PitchEnvSustain,
    
    PitchEnvRelease,
    
    Count
};
extern char const* gSynthParamTypeStrings[];
char const* SynthParamTypeToString(SynthParamType e);
SynthParamType StringToSynthParamType(char const* s);
char const* EnumToString(SynthParamType e);
void StringToEnum(char const* s, SynthParamType& e);

bool SynthParamTypeImGui(char const* label, SynthParamType* v);


}
