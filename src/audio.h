#pragma once

#include <iostream>

#include <portaudio.h>
#include "boost/circular_buffer.hpp"

#include "audio_util.h"
#include "synth.h"

#define NUM_SECONDS   (5)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)
#define NUM_OUTPUT_CHANNELS (2)

namespace audio {

int constexpr kNumSynths = 3;
int constexpr kNumPcmSounds = 3;
int constexpr kNumPcmVoices = 8;

struct PcmSound {
    float* _buffer = nullptr;
    uint64_t _bufferLength = 0;
};

struct PcmVoice {
    int _soundIx = -1;
    int _soundBufferIx = -1;
    float _gain = 1.f;
};

struct StateData {
    std::array<synth::StateData,kNumSynths> synths;

    std::array<PcmSound,kNumPcmSounds> pcmSounds;
    std::array<PcmVoice,kNumPcmVoices> pcmVoices;

    EventQueue* events = nullptr;
    boost::circular_buffer<Event> pendingEvents;

    char message[20];
};

void InitStateData(
    StateData& state, std::vector<synth::Patch> const& synthPatches, std::vector<PcmSound> const& pcmSounds,
    EventQueue* eventQueue, int sampleRate);

int PortAudioCallback(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData);

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
//    paTestData *data = (paTestData *) userData;
//    printf( "Stream Completed: %s\n", data->message );
    printf("Stream completed\n");
}

struct Context {
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
    Context& context, std::vector<synth::Patch> const& synthPatches, std::vector<PcmSound> const& pcmSounds);

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