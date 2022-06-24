#pragma once

#include "audio_util.h"
#include "synth.h"
#include "cereal/cereal.hpp"

#include "enums/audio_EventType_cereal.h"
#include "enums/audio_SynthParamType_cereal.h"

namespace audio {

template<typename Archive>
void serialize(Archive& ar, Event& e, std::uint32_t const version) {
    ar(cereal::make_nvp("type", e.type));
    ar(cereal::make_nvp("channel",e.channel));
    ar(cereal::make_nvp("tick_time",e.timeInTicks));
    if (e.type == EventType::SynthParam) {
        ar(cereal::make_nvp("synth_param", e.param));
        ar(cereal::make_nvp("value", e.newParamValue));
    } else {
        ar(cereal::make_nvp("midi_note", e.midiNote));
        if (version < 1) {
            e.velocity = 1.f;
        } else {
            ar(cereal::make_nvp("velocity", e.velocity));
        }
    }
}

} // namespace audio
CEREAL_CLASS_VERSION(audio::Event, 1);

namespace synth {

template<typename Archive>
void serialize(Archive& ar, ADSREnvSpec& s) {
    ar(CEREAL_NVP(s.attackTime), CEREAL_NVP(s.decayTime), CEREAL_NVP(s.sustainLevel),
     CEREAL_NVP(s.releaseTime), CEREAL_NVP(s.minValue));
}

template<typename Archive>
void serialize(Archive& ar, Patch& p) {
    ar(CEREAL_NVP(p.name), CEREAL_NVP(p.gainFactor), CEREAL_NVP(p.cutoffFreq), CEREAL_NVP(p.cutoffK), 
        CEREAL_NVP(p.pitchLFOGain), CEREAL_NVP(p.pitchLFOFreq), CEREAL_NVP(p.cutoffLFOGain),
        CEREAL_NVP(p.cutoffLFOFreq), CEREAL_NVP(p.ampEnvSpec), CEREAL_NVP(p.cutoffEnvSpec),
        CEREAL_NVP(p.cutoffEnvGain));
}

} // namespace synth