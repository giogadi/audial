#pragma once

#include "enums/audio_SynthParamType.h"
#include "enums/synth_Waveform.h"
#include "serial.h"

namespace synth {

struct ADSREnvSpec {
    float attackTime = 0.f;
    float decayTime = 0.f;
    float sustainLevel = 1.f;
    float releaseTime = 0.f;
    float minValue = 0.01f;

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
};

struct Patch {
    float const& Get(audio::SynthParamType paramType) const {
        return _data[static_cast<int>(paramType)];
    }
    float& Get(audio::SynthParamType paramType) {
        return _data[static_cast<int>(paramType)];
    }
    synth::Waveform GetOsc1Waveform() const;
    synth::Waveform GetOsc2Waveform() const;

    ADSREnvSpec GetAmpEnvSpec() const;
    ADSREnvSpec GetCutoffEnvSpec() const;
    ADSREnvSpec GetPitchEnvSpec() const;
   
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    // returns the param that changed (hopefully only one can change at a time...). Returns Count if none changed.
    struct ImGuiResult {
        bool _allChanged = false;
        audio::SynthParamType _changedParam = audio::SynthParamType::Count;
    };
    ImGuiResult ImGui();

    float _data[static_cast<int>(audio::SynthParamType::Count)];
};

}
