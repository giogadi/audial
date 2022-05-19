#pragma once

#include "audio_util.h"
#include "synth.h"
#include "cereal/cereal.hpp"

namespace audio {
template<typename Archive>
void save(Archive& ar, Event const& e) {
    ar(cereal::make_nvp("type",std::string(EventTypeToString(e.type))));
    ar(cereal::make_nvp("channel",e.channel));
    ar(cereal::make_nvp("tick_time",e.timeInTicks));
    if (e.type == EventType::SynthParam) {
        ar(cereal::make_nvp("synth_param", std::string(SynthParamTypeToString(e.param))));
        ar(cereal::make_nvp("value", e.newParamValue));
    } else {
        ar(cereal::make_nvp("midi_note", e.midiNote));
    }
}

template<typename Archive>
void load(Archive& ar, Event& e) {
    std::string eventTypeName;
    ar(eventTypeName);
    e.type = StringToEventType(eventTypeName.c_str());
    ar(cereal::make_nvp("channel",e.channel));
    ar(cereal::make_nvp("tick_time",e.timeInTicks));
    if (e.type == EventType::SynthParam) {
        std::string synthParamTypeName;
        ar(synthParamTypeName);
        e.param = StringToSynthParamType(synthParamTypeName.c_str());        
        ar(cereal::make_nvp("value", e.newParamValue));
    } else {
        ar(cereal::make_nvp("midi_note", e.midiNote));
    }
}

} // namespace audio

namespace synth {

template<typename Archive>
void serialize(Archive& ar, ADSREnvSpec& s) {
    ar(CEREAL_NVP(s.attackTime), CEREAL_NVP(s.decayTime), CEREAL_NVP(s.sustainLevel),
     CEREAL_NVP(s.releaseTime), CEREAL_NVP(s.minValue));
}

template<typename Archive>
void serialize(Archive& ar, Patch& p) {
    ar(CEREAL_NVP(p.gainFactor), CEREAL_NVP(p.cutoffFreq), CEREAL_NVP(p.cutoffK), 
        CEREAL_NVP(p.pitchLFOGain), CEREAL_NVP(p.pitchLFOFreq), CEREAL_NVP(p.cutoffLFOGain),
        CEREAL_NVP(p.cutoffLFOFreq), CEREAL_NVP(p.ampEnvSpec), CEREAL_NVP(p.cutoffEnvSpec),
        CEREAL_NVP(p.cutoffEnvGain));
}

} // namespace synth