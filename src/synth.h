#pragma once

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>

#include "boost/circular_buffer.hpp"

#include "audio_util.h"

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

        std::array<Voice, 4> voices;

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