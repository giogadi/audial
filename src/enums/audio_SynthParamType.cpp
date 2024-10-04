#include "src/enums/audio_SynthParamType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"


namespace audio {


namespace {

std::unordered_map<std::string, SynthParamType> const gStringToSynthParamType = {
    
    { "Gain", SynthParamType::Gain },
    
    { "Mono", SynthParamType::Mono },
    
    { "FM", SynthParamType::FM },
    
    { "Osc1Waveform", SynthParamType::Osc1Waveform },
    
    { "Osc2Waveform", SynthParamType::Osc2Waveform },
    
    { "Detune", SynthParamType::Detune },
    
    { "OscFader", SynthParamType::OscFader },
    
    { "Unison", SynthParamType::Unison },
    
    { "UnisonDetune", SynthParamType::UnisonDetune },
    
    { "Cutoff", SynthParamType::Cutoff },
    
    { "Peak", SynthParamType::Peak },
    
    { "HpfCutoff", SynthParamType::HpfCutoff },
    
    { "HpfPeak", SynthParamType::HpfPeak },
    
    { "Portamento", SynthParamType::Portamento },
    
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
    
    { "CutoffEnvRelease", SynthParamType::CutoffEnvRelease },
    
    { "PitchEnvGain", SynthParamType::PitchEnvGain },
    
    { "PitchEnvAttack", SynthParamType::PitchEnvAttack },
    
    { "PitchEnvDecay", SynthParamType::PitchEnvDecay },
    
    { "PitchEnvSustain", SynthParamType::PitchEnvSustain },
    
    { "PitchEnvRelease", SynthParamType::PitchEnvRelease },
    
    { "FMOsc2Level", SynthParamType::FMOsc2Level },
    
    { "FMOsc2Ratio", SynthParamType::FMOsc2Ratio },
    
    { "DelayGain", SynthParamType::DelayGain },
    
    { "DelayTime", SynthParamType::DelayTime },
    
    { "DelayFeedback", SynthParamType::DelayFeedback }
    
};

} // end namespace

char const* gSynthParamTypeStrings[] = {
	
    "Gain",
    
    "Mono",
    
    "FM",
    
    "Osc1Waveform",
    
    "Osc2Waveform",
    
    "Detune",
    
    "OscFader",
    
    "Unison",
    
    "UnisonDetune",
    
    "Cutoff",
    
    "Peak",
    
    "HpfCutoff",
    
    "HpfPeak",
    
    "Portamento",
    
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
    
    "CutoffEnvRelease",
    
    "PitchEnvGain",
    
    "PitchEnvAttack",
    
    "PitchEnvDecay",
    
    "PitchEnvSustain",
    
    "PitchEnvRelease",
    
    "FMOsc2Level",
    
    "FMOsc2Ratio",
    
    "DelayGain",
    
    "DelayTime",
    
    "DelayFeedback"
    
};

char const* SynthParamTypeToString(SynthParamType e) {
	return gSynthParamTypeStrings[static_cast<int>(e)];
}

SynthParamType StringToSynthParamType(char const* s) {
    auto iter = gStringToSynthParamType.find(s);
    if (iter != gStringToSynthParamType.end()) {
    	return gStringToSynthParamType.at(s);
    }
    printf("ERROR StringToSynthParamType: unrecognized value \"%s\"\n", s);
    return static_cast<SynthParamType>(0);
}

bool SynthParamTypeImGui(char const* label, SynthParamType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gSynthParamTypeStrings, static_cast<int>(SynthParamType::Count));
    if (changed) {
        *v = static_cast<SynthParamType>(selectedIx);
    }
    return changed;
}


}

char const* EnumToString(audio::SynthParamType e) {
     return audio::SynthParamTypeToString(e);
}

void StringToEnum(char const* s, audio::SynthParamType& e) {
     e = audio::StringToSynthParamType(s);
}