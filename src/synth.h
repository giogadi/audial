#pragma once

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>

#include "boost/circular_buffer.hpp"

#include "rng.h"
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
    int64_t ticksSincePhaseStart = 0;
    float value = 0.f;
    float attackCoeff = 0.f;
    float attackOffset = 0.f;
    float attackTCO = 0.f;

    float decayCoeff = 0.f;
    float decayOffset = 0.f;
    float decayTCO = 0.f;

    float releaseCoeff = 0.f;
    float releaseOffset = 0.f;
    float releaseTCO = 0.f;
};

int constexpr kMaxNumOscillators = 6;
int constexpr kNumAnalogOscillators = 2;
int constexpr kMaxUnison = 5;

struct Oscillator {
    float f = 440.f;
    float phases[kMaxUnison];
    rng::State rng;
};

struct FilterState {
    float ic1eq = 0.f;
    float ic2eq = 0.f;
};

struct Voice {
    // We assign the voice's "center" frequency using oscillators[0]. I know, this is gross.
    // If this is an analog voice, only the first kNumAnalogOscillators are used.
    std::array<Oscillator, kMaxNumOscillators> oscillators;

    ADSRStateNew ampEnvState;
    ADSRStateNew cutoffEnvState;
    ADSREnvState pitchEnvState;
    int currentMidiNote = -1;
    float velocity = 1.f;
    int noteOnId = 0;
    float postPortamentoF = 0.f;  // latest output of applying porta to center freq.

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
    int64_t modulationHz = 1;
    int64_t attackTime = 0;
    int64_t decayTime = 0;
    float sustainLevel = 0.f;
    int64_t releaseTime = 0;

    float attackCoeff = 0.f;
    float attackOffset = 0.f;
    float attackTCO = 0.f;

    float decayCoeff = 0.f;
    float decayOffset = 0.f;
    float decayTCO = 0.f;

    float releaseCoeff = 0.f;
    float releaseOffset = 0.f;
    float releaseTCO = 0.f;
};

struct StateData {
    int channel = -1;

    std::array<Voice, 4> voices;
    std::array<Automation, 64> automations;

    Patch patch;

    float pitchLFOPhase = 0.0f;
    float cutoffLFOPhase = 0.0f;

    float* voiceScratchBuffer = nullptr;
    float* synthScratchBuffer = nullptr;

    int sampleRate;
    int framesPerBuffer;

    ADSREnvSpecInternal ampEnvSpecInternal;
    ADSREnvSpecInternal cutoffEnvSpecInternal;
    int samplesPerMoogCutoffUpdate = 1;

    // includes all channels, interleaved
    float* delayBuffer = nullptr;
    int delayBufferWriteIx = 0;
};

void InitStateData(StateData& state, int channel, int const sampleRate, int const samplesPerFrame, int const numBufferChannels);
void DestroyStateData(StateData& state);

void NoteOn(StateData& state, int midiNote, float velocity, int noteOnId = 0);
void NoteOff(StateData& state, int midiNote, int noteOffId = 0);
void AllNotesOff(StateData& state);

void Process(
    StateData* state, audio::PendingEvent *eventsThisFrame, int eventsThisFrameCount,
    float* outputBuffer, int numChannels, int framesPerBuffer,
    int sampleRate, int64_t currentBufferCounter);
}
