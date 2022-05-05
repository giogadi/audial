#pragma once

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "SPSCQueue.h"

namespace synth {
    static inline float const kPi = 3.141592653589793f;

    static inline float const kSemitoneRatio = 1.05946309f;
    static inline float const kFifthRatio = 1.49830703f;

    static inline float const kSmallAmplitude = 0.0001f;

    // TODO: use a lookup table!!!
    inline float MidiToFreq(int midi) {
        int const a4 = 69;  // 440Hz
        return 440.0f * pow(2.0f, (midi - a4) / 12.0f);
    }

    enum class EventType {
        None, NoteOn, NoteOff
    };

    struct Event {
        EventType type;
        int timeInTicks = 0;
        int midiNote = 0;
    };

    enum class AdsrState {
        Closed, Opening, Closing
    };

    static inline int const kEventQueueLength = 64;
    typedef rigtorp::SPSCQueue<Event> EventQueue;

    struct StateData {
        float f = 440.0f;
        float left_phase = 0.0f;
        float right_phase = 0.0f;
        float cutoffFreq = 0.0f;
        float cutoffK = 0.0f;  // [0,4] but 4 is unstable
        float lp0 = 0.0f;
        float lp1 = 0.0f;
        float lp2 = 0.0f;
        float lp3 = 0.0f;

        float pitchLFOGain = 0.0f;
        float pitchLFOFreq = 0.0f;
        float pitchLFOPhase = 0.0f;

        float cutoffLFOGain = 0.0f;
        float cutoffLFOFreq = 0.0f;
        float cutoffLFOPhase = 0.0f;

        float ampEnvAttackTime = 0.0f;
        float ampEnvDecayTime = 0.0f;
        float ampEnvSustainLevel = 1.0f;
        float ampEnvReleaseTime = 0.0f;
        int ampEnvTicksSinceStart = -1;
        AdsrState ampEnvState = AdsrState::Closed;
        float lastNoteOnAmpEnvValue = 0.0f;

        EventQueue* events = nullptr;

        int tickTime = 0;

        char message[20];
    };

    

    void InitStateData(StateData& state, EventQueue* eventQueue, int sampleRate);

    void InitEventQueueWithSequence(EventQueue* queue, int sampleRate);

    void Process(StateData* state, float* outputBuffer, int const numChannels, int const framesPerBuffer, int const sampleRate);
}