#pragma once

#include <iostream>
#include <portaudio.h>

#include "audio_util.h"
#include "synth.h"

#define NUM_SECONDS   (5)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)
#define NUM_OUTPUT_CHANNELS (2)

namespace audio {

struct StateData {
    synth::StateData synth;

    float* pcmBuffer = nullptr;
    int pcmBufferLength = 0;
    int currentPcmBufferIx = 0;

    EventQueue* events = nullptr;

    char message[20];
};

void InitStateData(StateData& state, EventQueue* eventQueue, int sampleRate, float* pcmBuffer, unsigned long pcmBufferLength);

void InitEventQueueWithSequence(EventQueue* queue, int sampleRate);

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
};

PaError Init(Context& context, float* pcmBuffer, unsigned long pcmBufferLength);

PaError ShutDown(Context& stream);

}  // namespace audio