#pragma once

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>

#include "boost/circular_buffer.hpp"

#include "audio_util.h"
#include "serial.h"
#include "enums/synth_Waveform.h"

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
    float sustainLevel = 1.f;
    float releaseTime = 0.f;
    float minValue = 0.01f;

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
};

struct Patch {
    std::string name;
    // This is interpreted linearly from [0,1]. This will get mapped later to [-80db,0db].
    // TODO: consider just having this be a decibel value capped at 0db.
    float gainFactor = 0.7f;

    bool mono = false;

    Waveform osc1Waveform = Waveform::Square;
    Waveform osc2Waveform = Waveform::Square;

    float detune = 0.f;   // 1.0f is a whole octave.
    float oscFader = 0.5f;  // 0.f is fully Osc1, 1.f is fully Osc2.

    float cutoffFreq = 44100.0f;
    float cutoffK = 0.0f;  // [0,4] but 4 is unstable

    float hpfCutoffFreq = 0.0f;
    float hpfPeak = 0.0f;

    // gain of 1.0 coincides with the wave varying across 2 octaves (one octave up and one octave down).
    float pitchLFOGain = 0.0f;
    float pitchLFOFreq = 0.0f;

    // gain of 1.0 coincides with cutoff frequency doubling at LFO peak and halving at LFO valley.
    float cutoffLFOGain = 0.0f;
    float cutoffLFOFreq = 0.0f;

    ADSREnvSpec ampEnvSpec;

    ADSREnvSpec cutoffEnvSpec;
    float cutoffEnvGain = 0.f;

    ADSREnvSpec pitchEnvSpec;
    float pitchEnvGain = 0.f;

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);

    static int constexpr kVersion = 4;
};

int constexpr kNumOscillators = 2;

struct Oscillator {
    float f = 440.f;
    float phase = 0.f;
};

struct FilterState {
    float ic1eq = 0.f;
    float ic2eq = 0.f;
};

struct Voice {
    // We assign the voice's "center" frequency using oscillators[0]. I know, this is gross.
    std::array<Oscillator, kNumOscillators> oscillators;

    ADSREnvState ampEnvState;
    ADSREnvState cutoffEnvState;
    ADSREnvState pitchEnvState;
    int currentMidiNote = -1;
    float velocity = 1.f;

    // float lp0 = 0.0f;
    // float lp1 = 0.0f;
    // float lp2 = 0.0f;
    // float lp3 = 0.0f;

    FilterState lpfState;
    FilterState hpfState;
};

struct Automation {
    bool _active = false;
    audio::SynthParamType _synthParamType;
    float _startValue = 0.f;
    float _desiredValue = 0.f;
    long _startTickTime = 0;
    long _endTickTime = 0;    
};

struct StateData {
    int channel = -1;

    std::array<Voice, 6> voices;
    std::array<Automation, 20> automations;

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
