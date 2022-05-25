#include "audio.h"

#include <array>
#include <chrono>
#include <vector>

#include "audio_util.h"
#include "synth.h"

using std::chrono::high_resolution_clock;

namespace audio {

namespace {

void OnPortAudioError(PaError const& err) {
    Pa_Terminate();
    std::cout << "An error occured while using the portaudio stream" << std::endl;
    std::cout << "Error number: " << err << std::endl;
    std::cout << "Error message: " << Pa_GetErrorText(err) << std::endl;
}

} // namespace

void InitStateData(
    StateData& state, std::vector<synth::Patch> const& synthPatches, EventQueue* eventQueue,
    int sampleRate, float* pcmBuffer, unsigned long pcmBufferLength) {

    bool useInputPatches = true;
    if (synthPatches.size() != state.synths.size()) {
        useInputPatches = false;
        std::cout << "Missing/mismatched patch data. Loading hardcoded default patch data." << std::endl;
    }

    for (int i = 0; i < state.synths.size(); ++i) {
        synth::StateData& s = state.synths[i];
        synth::InitStateData(s, /*channel=*/i);
        if (useInputPatches) {
            s.patch = synthPatches[i];
        }
    }

    state.pcmBuffer = pcmBuffer;
    state.pcmBufferLength = pcmBufferLength;
    state.currentPcmBufferIx = -1;

    state.events = eventQueue;
}

PaError Init(
    Context& context, std::vector<synth::Patch> const& synthPatches, float* pcmBuffer,
    unsigned long pcmBufferLength) {

    PaError err;
    
    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    InitStateData(context._state, synthPatches, &context._eventQueue, SAMPLE_RATE, pcmBuffer, pcmBufferLength);

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

void ProcessEventQueue(EventQueue* eventQueue, std::array<Event, 256>* pendingEvents, int& pendingEventCount, long tickTime) {
    for (; pendingEventCount < pendingEvents->size(); ++pendingEventCount) {
        Event *e = eventQueue->front();
        if (e == nullptr) {
            return;
        }
        (*pendingEvents)[pendingEventCount] = *e;
        eventQueue->pop();
    }
    if (pendingEventCount == pendingEvents->size()) {
        std::cout << "WARNING: WE FILLED THE PENDING EVENT LIST" << std::endl;
    }
}

int FindEventsInThisFrame(
    std::array<Event, 256>* pendingEvents, int& pendingEventCount, unsigned long framesPerBuffer,
    double const timeSecs, EventsThisFrame* eventsThisFrame) {
    
    int numEventsThisFrame = 0;
    long const frameStartTickTime = timeSecs * SAMPLE_RATE;
    for (int pendingEventIx = 0; pendingEventIx < pendingEventCount; /*no increment*/) {
        if (numEventsThisFrame >= eventsThisFrame->size()) {
            break;
        }

        Event& e = (*pendingEvents)[pendingEventIx];
        long sampleIx = e.timeInTicks - frameStartTickTime;
        if (sampleIx < 0) {
            // remove this element by swapping the last element with this one.
            // printf("WARNING: PROCESSED A STALE EVENT: %ld   %ld    %f\n", sampleIx, e.timeInTicks, timeSecs);
            // std::swap(e, (*pendingEvents)[pendingEventCount-1]);
            // --pendingEventCount;
            // continue;
            sampleIx = 0;
        }
        if (sampleIx >= framesPerBuffer) {
            // not time yet. move to the next frame.
            ++pendingEventIx;
            continue;
        }
        // Another copy, YUCK.
        FrameEvent& fe = (*eventsThisFrame)[numEventsThisFrame];
        fe._e = e;
        fe._sampleIx = sampleIx;
        ++numEventsThisFrame;

        // Remove this element from pendingEvents.
        std::swap(e, (*pendingEvents)[pendingEventCount-1]);
        --pendingEventCount;
    }

    return numEventsThisFrame;
}

namespace {
    double gLastCallbackTime = 0.0;
    int gNumDesyncs = 0;
    int gDevSum = 0;
    int gTotalCallbacks = 0;

    double gTimeSinceLastCallback = 0.0;
    double gTotalTime = 0.0;
    unsigned long gLastFrameSize = 0;
}

int GetNumDesyncs() {
    return gNumDesyncs;
}

int GetAvgDesyncTickTime() {
    return gDevSum / gTotalCallbacks;
}

double GetAvgTimeBetweenCallbacks() {
    return gTotalTime / gTotalCallbacks;
}

double GetLastDt() {
    return gTimeSinceLastCallback;
}

unsigned long GetLastFrameSize() {
    return gLastFrameSize;
}

int PortAudioCallback(
    const void *inputBuffer, void *const outputBufferUntyped,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData) {

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    gLastFrameSize = framesPerBuffer;

    StateData* state = (StateData*)userData;
    double const timeSecs = timeInfo->currentTime;
    // double const timeSecs = timeInfo->outputBufferDacTime;

    // DEBUG
    gTimeSinceLastCallback = timeSecs - gLastCallbackTime;
    gTotalTime += gTimeSinceLastCallback;
    long ticksSinceLastCallback = (long)(gTimeSinceLastCallback * SAMPLE_RATE);
    if (ticksSinceLastCallback != framesPerBuffer) {
        ++gNumDesyncs;
    }
    gDevSum += (int)(std::abs(ticksSinceLastCallback - (long)framesPerBuffer));
    ++gTotalCallbacks;
    gLastCallbackTime = timeSecs;
    // DEBUG
    
    // zero out the output buffer first.
    memset(outputBufferUntyped, 0, NUM_OUTPUT_CHANNELS * framesPerBuffer * sizeof(float));

    // Figure out which events apply to this invocation of the callback, and which effects should handle them.
    ProcessEventQueue(state->events, &state->pendingEvents, state->pendingEventCount, timeSecs * SAMPLE_RATE);
    EventsThisFrame eventsThisFrame;
    int const numEventsThisFrame = FindEventsInThisFrame(&(state->pendingEvents), state->pendingEventCount, framesPerBuffer, timeSecs, &eventsThisFrame);

    float* outputBuffer = (float*) outputBufferUntyped;

    // PCM playback first
    int frameEventIx = 0;
    for (int i = 0; i < framesPerBuffer; ++i) {
        // Handle events
        while (true) {
            if (frameEventIx >= numEventsThisFrame) {
                break;
            }
            // PendingEvent const& e = pendingEvents[pendingEventIx];
            FrameEvent const& fe = eventsThisFrame[frameEventIx];
            if (fe._sampleIx == i) {
                switch (fe._e.type) {
                    case EventType::PlayPcm: {
                        state->currentPcmBufferIx = 0;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                ++frameEventIx;
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

    for (synth::StateData& s : state->synths) {
        synth::Process(
            &s, eventsThisFrame, numEventsThisFrame, outputBuffer,
            NUM_OUTPUT_CHANNELS, framesPerBuffer, SAMPLE_RATE);
    }
    
    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    const double kSecsPerCallback = (double) framesPerBuffer / SAMPLE_RATE;
    double callbackTimeSecs = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    if (callbackTimeSecs / kSecsPerCallback > 0.9) {
        printf("Frame close to deadline: %f / %f", callbackTimeSecs, kSecsPerCallback);
    }
        
    return paContinue;
}

}  // namespace audio