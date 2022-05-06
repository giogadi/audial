#include "audio.h"
#include "audio_util.h"
#include "synth.h"

#include <array>

namespace audio {

namespace {

void OnPortAudioError(PaError const& err) {
    Pa_Terminate();
    std::cout << "An error occured while using the portaudio stream" << std::endl;
    std::cout << "Error number: " << err << std::endl;
    std::cout << "Error message: " << Pa_GetErrorText(err) << std::endl;
}

} // namespace

void InitStateData(StateData& state, EventQueue* eventQueue, int sampleRate, float* pcmBuffer, unsigned long pcmBufferLength) {
    synth::InitStateData(state.synth);

    state.pcmBuffer = pcmBuffer;
    state.pcmBufferLength = pcmBufferLength;
    state.currentPcmBufferIx = -1;

    state.events = eventQueue;

    // InitEventQueueWithSequence(eventQueue, sampleRate);
}

void InitEventQueueWithSequence(EventQueue* queue, int sampleRate) {
    int const bpm = 200;
    int const kSamplesPerBeat = (sampleRate * 60) / bpm;
    for (int i = 0; i < 16; ++i) {
        Event e;
        e.type = EventType::NoteOn;
        e.timeInTicks = kSamplesPerBeat*i;
        e.midiNote = 69 + i;
        queue->push(e);

        e.type = EventType::NoteOff;
        e.timeInTicks = kSamplesPerBeat*i + (kSamplesPerBeat / 2);
        queue->push(e);
    }
}

PaError Init(Context& context, float* pcmBuffer, unsigned long pcmBufferLength)
{
    PaError err;
    
    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    // InitEventQueueWithSequence(&context._eventQueue, SAMPLE_RATE);
    InitStateData(context._state, &context._eventQueue, SAMPLE_RATE, pcmBuffer, pcmBufferLength);

    err = Pa_Initialize();
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    context._outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (context._outputParameters.device == paNoDevice) {
      printf("Error: No default output device.\n");
      return err;
    }
    context._outputParameters.channelCount = 2;       /* stereo output */
    context._outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    context._outputParameters.suggestedLatency = Pa_GetDeviceInfo( context._outputParameters.device )->defaultLowOutputLatency;
    context._outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &context._stream,
              NULL, /* no input */
              &context._outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              PortAudioCallback,
              &context._state );
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_SetStreamFinishedCallback( context._stream, &StreamFinished );
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_StartStream( context._stream );
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    return paNoError;
}

PaError ShutDown(Context& context) {
    PaError err = Pa_StopStream(context._stream);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_CloseStream(context._stream);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    Pa_Terminate();
    return paNoError;
}

// TODO: We don't need to go through every frame of the buffer!!!!!!
int ProcessEventQueue(EventQueue* eventQueue, unsigned long framesPerBuffer, double const timeSecs, PendingEventBuffer* pendingEvents) {
    long tickTime = timeSecs * SAMPLE_RATE;
    int pendingEventCount = 0;
    for (int i = 0; i < framesPerBuffer; ++i) {
        Event* e = eventQueue->front();
        while (e != nullptr && tickTime >= e->timeInTicks) {
            if (e->timeInTicks == tickTime) {
                PendingEvent& pending = (*pendingEvents)[pendingEventCount];
                pending._e = *e;
                pending._sampleIx = i;
                ++pendingEventCount;
                if (pendingEventCount >= pendingEvents->size()) {
                    std::cout << "MAXED OUT PENDING EVENT BUFFER!" << std::endl;
                    return pendingEvents->size();
                }
            }
            eventQueue->pop();
            e = eventQueue->front();
        }
        if (e == nullptr) {
            break;
        }
        ++tickTime;
    }
    return pendingEventCount;
}

int PortAudioCallback(
    const void *inputBuffer, void *const outputBufferUntyped,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData) {
    StateData* state = (StateData*)userData;
    double const timeSecs = timeInfo->currentTime;
    
    // zero out the output buffer first.
    memset(outputBufferUntyped, 0, NUM_OUTPUT_CHANNELS * framesPerBuffer * sizeof(float));

    // Figure out which events apply to this invocation of the callback, and which effects should handle them.
    PendingEventBuffer pendingEvents;
    int const eventCount = ProcessEventQueue(state->events, framesPerBuffer, timeSecs, &pendingEvents);

    float* outputBuffer = (float*) outputBufferUntyped;

    // PCM playback first
    int pendingEventIx = 0;
    for (int i = 0; i < framesPerBuffer; ++i) {
        // Handle events
        while (true) {
            if (pendingEventIx >= eventCount) {
                break;
            }
            PendingEvent const& e = pendingEvents[pendingEventIx];
            if (e._sampleIx == i) {
                switch (e._e.type) {
                    case EventType::PlayPcm: {
                        state->currentPcmBufferIx = 0;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                ++pendingEventIx;
            } else {
                break;
            }
        }

        float v = 0.0f;
        if (state->currentPcmBufferIx >= 0 &&
            state->currentPcmBufferIx < state->pcmBufferLength) {
            v = state->pcmBuffer[state->currentPcmBufferIx];
            ++state->currentPcmBufferIx;
        }

        for (int channelIx = 0; channelIx < NUM_OUTPUT_CHANNELS; ++channelIx) {
            *outputBuffer++ += v;
        }
    }

    outputBuffer = (float*) outputBufferUntyped;

    synth::Process(
        &state->synth, pendingEvents, eventCount, outputBuffer,
        NUM_OUTPUT_CHANNELS, framesPerBuffer, SAMPLE_RATE);
        
    return paContinue;
}

}  // namespace audio