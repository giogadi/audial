#pragma once

#include <iostream>
#include <vector>
#include <mutex>

#include <portaudio.h>

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
    int sampleRate = -1;

    std::array<synth::StateData,kNumSynths> synths;

    SoundBank const* soundBank = nullptr;
    std::array<PcmVoice,kNumPcmVoices> pcmVoices;

    EventQueue* events = nullptr;
    PendingEventHeap pendingEvents;

    float _finalGain = 1.f;

    int64_t _bufferCounter = 0;

    std::mutex _recentBufferMutex;
    int _bufferFrameCount = 0;
    float* _recentBuffer = nullptr;
};

void InitStateData(
    StateData& state,
    SoundBank const& soundBank,
    EventQueue* eventQueue, int sampleRate);
void DestroyStateData(StateData& state);

int PortAudioCallback(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData);

struct Context {
    int _sampleRate = -1;
    PaStreamParameters _outputParameters;
    PaStream* _stream = nullptr;
    StateData _state;
    EventQueue _eventQueue;
    Context()
        : _eventQueue(kEventQueueLength) {}

    bool AddEvent(Event const& e) {
        bool success = _eventQueue.try_push(e);
        if (!success) {
            // TODO: maybe use serialize to get a string of the event
            std::cout << "Failed to add event to audio queue:" << std::endl;
        }
        return success;
    }
};

PaError Init(
    Context& context, SoundBank const& soundBank);

PaError ShutDown(Context& stream);

int GetNumDesyncs();
int GetAvgDesyncTime();
double GetAvgTimeBetweenCallbacks();
double GetLastDt();
unsigned long GetLastFrameSize();

inline void PrintAudioStats() {
    // printf("Num desyncs: %d\n", audio::GetNumDesyncs());
    // printf("Avg desync time: %d\n", audio::GetAvgDesyncTime());
    // printf("Avg dt: %f\n", audio::GetAvgTimeBetweenCallbacks() * SAMPLE_RATE);
    // printf("dt: %f\n", audio::GetLastDt());
    // std::cout.precision(std::numeric_limits<double>::max_digits10);
    // std::cout << "dt: " << audio::GetLastDt() << std::endl;
    // printf("frame size: %lu\n", audio::GetLastFrameSize());
}

}  // namespace audio
