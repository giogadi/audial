#pragma once

#include <iostream>
#include <portaudio.h>

#include "synth.h"

#define NUM_SECONDS   (5)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

static int Callback(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData) {
    synth::StateData* state = (synth::StateData*)userData;
    synth::Process(state, (float*)outputBuffer, /*numChannels=*/2, framesPerBuffer, SAMPLE_RATE);
    return paContinue;
}

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
//    paTestData *data = (paTestData *) userData;
//    printf( "Stream Completed: %s\n", data->message );
    printf("Stream completed\n");
}

void OnPortAudioError(PaError const& err);

struct PortAudioContext {
    PaStreamParameters _outputParameters;
    PaStream* _stream = nullptr;
    synth::StateData _state;
    synth::EventQueue _eventQueue;
    PortAudioContext()
        : _eventQueue(synth::kEventQueueLength) {}

};

PaError InitPortAudio(PortAudioContext& context);

PaError ShutDownPortAudio(PaStream* stream);