#pragma once

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>

#include "boost/circular_buffer.hpp"

#include "audio_util.h"
#include "serial.h"

using boost::property_tree::ptree;

namespace synth {
    static inline float const kSemitoneRatio = 1.05946309f;
    static inline float const kFifthRatio = 1.49830703f;

    static inline float const kSmallAmplitude = 0.0001f;

    // TODO: use a lookup table!!!
    inline float MidiToFreq(int midi) {
        int const a4 = 69;  // 440Hz
        return 440.0f * pow(2.0f, (midi - a4) / 12.0f);
    }

    enum class ADSRPhase {
        Closed, Attack, Decay, Sustain, Release
    };

    struct ADSREnvState {
        ADSRPhase phase = ADSRPhase::Closed;
        float currentValue = 0.f;
        float multiplier = 0.f;
        long ticksSincePhaseStart = -1;
    };

    struct ADSREnvSpec {
        float attackTime = 0.f;
        float decayTime = 0.f;
        float sustainLevel = 0.f;
        float releaseTime = 0.f;
        float minValue = kSmallAmplitude;

        void Save(ptree& pt) const {
            pt.put("attack_time", attackTime);
            pt.put("decay_time", decayTime);
            pt.put("sustain_level", sustainLevel);
            pt.put("release_time", releaseTime);
            pt.put("min_value", minValue);
        }
        void Load(ptree const& pt) {
            attackTime = pt.get<float>("attack_time");
            decayTime = pt.get<float>("decay_time");
            sustainLevel = pt.get<float>("sustain_level");
            releaseTime = pt.get<float>("release_time");
            minValue = pt.get<float>("min_value");
        }
    };

    struct Patch {
        std::string name;
        // This is interpreted linearly from [0,1]. This will get mapped later to [-80db,0db].
        // TODO: consider just having this be a decibel value capped at 0db.
        float gainFactor = 0.5f;

        float cutoffFreq = 0.0f;
        float cutoffK = 0.0f;  // [0,4] but 4 is unstable

        float pitchLFOGain = 0.0f;
        float pitchLFOFreq = 0.0f;

        float cutoffLFOGain = 0.0f;
        float cutoffLFOFreq = 0.0f;

        ADSREnvSpec ampEnvSpec;

        ADSREnvSpec cutoffEnvSpec;
        float cutoffEnvGain = 0.f;

        void Save(ptree& pt) const {
            pt.put("name", name);
            pt.put("gain_factor", gainFactor);
            pt.put("cutoff_freq", cutoffFreq);
            pt.put("cutoff_k", cutoffK);
            pt.put("pitch_lfo_gain", pitchLFOGain);
            pt.put("pitch_lfo_freq", pitchLFOFreq);
            pt.put("cutoff_lfo_gain", cutoffLFOGain);
            pt.put("cutoff_lfo_freq", cutoffLFOFreq);
            serial::SaveInNewChildOf(pt, "amp_env_spec", ampEnvSpec);
            pt.put("cutoff_env_gain", cutoffEnvGain);
            serial::SaveInNewChildOf(pt, "cutoff_env_spec", cutoffEnvSpec);
        }
        void Load(ptree const& pt) {
            name = pt.get<std::string>("name");
            gainFactor = pt.get<float>("gain_factor");
            cutoffFreq = pt.get<float>("cutoff_freq");
            cutoffK = pt.get<float>("cutoff_k");
            pitchLFOGain = pt.get<float>("pitch_lfo_gain");
            pitchLFOFreq = pt.get<float>("pitch_lfo_freq");
            cutoffLFOGain = pt.get<float>("cutoff_lfo_gain");
            cutoffLFOFreq = pt.get<float>("cutoff_lfo_freq");
            ampEnvSpec.Load(pt.get_child("amp_env_spec"));
            cutoffEnvGain = pt.get<float>("cutoff_env_gain");
            cutoffEnvSpec.Load(pt.get_child("cutoff_env_spec"));
        }
    };

    struct Voice {
        float f = 440.0f;
        float left_phase = 0.0f;
        float right_phase = 0.0f;

        ADSREnvState ampEnvState;
        ADSREnvState cutoffEnvState;
        int currentMidiNote = -1;

        float lp0 = 0.0f;
        float lp1 = 0.0f;
        float lp2 = 0.0f;
        float lp3 = 0.0f;
    };

    struct StateData {
        int channel = -1;

        std::array<Voice, 6> voices;

        Patch patch;

        float pitchLFOPhase = 0.0f;
        float cutoffLFOPhase = 0.0f;
    };

    void InitStateData(StateData& state, int channel);

    void Process(
        StateData* state, boost::circular_buffer<audio::Event> const& pendingEvents,
        float* outputBuffer, int const numChannels, int const framesPerBuffer,
        int const sampleRate, unsigned long frameStartTickTime);
}