#pragma once


namespace audio {


enum class SynthParamType : int {
    
    Gain,
    
    Mono,
    
    FM,
    
    Osc1Waveform,
    
    Osc2Waveform,
    
    Detune,
    
    OscFader,
    
    Unison,
    
    UnisonDetune,
    
    Cutoff,
    
    Peak,
    
    HpfCutoff,
    
    HpfPeak,
    
    Portamento,
    
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
    
    FMOsc2Level,
    
    FMOsc2Ratio,
    
    DelayGain,
    
    DelayTime,
    
    DelayFeedback,
    
    Count
};
extern char const* gSynthParamTypeStrings[];
char const* SynthParamTypeToString(SynthParamType e);
SynthParamType StringToSynthParamType(char const* s);

bool SynthParamTypeImGui(char const* label, SynthParamType* v);


}

char const* EnumToString(audio::SynthParamType e);
void StringToEnum(char const* s, audio::SynthParamType& e);