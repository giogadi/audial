#pragma once

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>

#include "boost/circular_buffer.hpp"

#include "audio_util.h"
#include "serial.h"
#include "enums/synth_Waveform.h"
#include "synth_patch.h"
#include "filter.h"

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
    int64_t ticksSincePhaseStart = -1;
};

struct ADSRStateNew {
    ADSRPhase phase = ADSRPhase::Closed;
    float value = 0.f;
    float multiplier = 0.f;
    float atkAuxVal = 0.f;
    float targetLevel = 0.f;
    int64_t ticksSincePhaseStart = 0;
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

    ADSRStateNew ampEnvState;
    ADSRStateNew cutoffEnvState;
    ADSREnvState pitchEnvState;
    int currentMidiNote = -1;
    float velocity = 1.f;
    int noteOnId = 0;
    float postPortamentoF = 0.f;  // latest output of applying porta to center freq.

    FilterState lpfState;
    FilterState hpfState;

    filter::VAMoogFilter moogLpfState;
};

struct Automation {
    bool _active = false;
    audio::SynthParamType _synthParamType;
    float _startValue = 0.f;
    float _desiredValue = 0.f;
    int64_t _startTickTime = 0;
    int64_t _endTickTime = 0;    
};

// All times are in "modulation steps". Could be samples, buffers, whatever the caller wants.
struct ADSREnvSpecInternal {
    float modulationHz = 1.f;
    float attackTime = 0.f;
    float decayTime = 0.f;
    float sustainLevel = 0.f;
    float releaseTime = 0.f;
};

struct StateData {
    int channel = -1;

    std::array<Voice, 4> voices;
    std::array<Automation, 64> automations;

    Patch patch;

    float pitchLFOPhase = 0.0f;
    float cutoffLFOPhase = 0.0f;

    float* voiceScratchBuffer = nullptr;

    int sampleRate;
    int framesPerBuffer;

    ADSREnvSpecInternal ampEnvSpecInternal;
    ADSREnvSpecInternal cutoffEnvSpecInternal;
};

void InitStateData(StateData& state, int channel, int const sampleRate, int const samplesPerFrame, int const numBufferChannels);
void DestroyStateData(StateData& state);

void NoteOn(StateData& state, int midiNote, float velocity, int noteOnId = 0);
void NoteOff(StateData& state, int midiNote, int noteOffId = 0);
void AllNotesOff(StateData& state);

void Process(
    StateData* state, boost::circular_buffer<audio::PendingEvent> const& pendingEvents,
    float* outputBuffer, int numChannels, int framesPerBuffer,
    int sampleRate, int64_t currentBufferCounter);
}
