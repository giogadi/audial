#pragma once

#include <iostream>
#include <vector>
#include <mutex>

#include "audio_util.h"
#include "synth.h"

class SoundBank;

namespace audio {

int constexpr kNumSynths = 5;
int constexpr kNumPcmVoices = 8;

struct PcmVoice {
    int _soundIx = -1;
    int _soundBufferIx = -1;
    float _gain = 1.f;
    bool _loop = false;
};

struct HeapEntry {
    PendingEvent e;
    int key;
    int counter;
};

struct PendingEventHeap {
    HeapEntry *entries = nullptr;
    int size = 0;
    int counter = 0;
    int maxSize = 0;
};

struct StateData {
    int outputSampleRate = -1;

    std::array<synth::StateData,kNumSynths> synths;

    SoundBank const* soundBank = nullptr;
    std::array<PcmVoice,kNumPcmVoices> pcmVoices;

    PendingEventHeap pendingEvents;

    float _finalGain = 1.f;

    int64_t _bufferCounter = 0;

    std::mutex _recentBufferMutex;
    int _bufferFrameCount = 0;
    float* _recentBuffer = nullptr;
};

void InitStateData(StateData& state, SoundBank const& soundBank, int outputSampleRate, int framesPerBuffer);
void DestroyStateData(StateData& state);

void AudioCallback(float const* inputBuffer, float *outputBuffer, unsigned long framesPerBuffer, StateData *state);

bool AddEvent(Event const& e);

int InternalSampleRate();

int NumOutputChannels();


}  // namespace audio
