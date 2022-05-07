#include "audio.h"

#include <array>
#include <chrono>

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
    StateData& state, EventQueue* eventQueue, int sampleRate, float* pcmBuffer,
    unsigned long pcmBufferLength) {
    synth::InitStateData(state.synth);

    state.pcmBuffer = pcmBuffer;
    state.pcmBufferLength = pcmBufferLength;
    state.currentPcmBufferIx = -1;

    state.events = eventQueue;
}

void InitEventQueueWithSequence(EventQueue* queue, int sampleRate, double const audioTime) {
    constexpr double kBpm = 120.0;
    double const currentBeatTime = audioTime * (kBpm / 60.0);
    double noteBeatTime = floor(currentBeatTime) + 2.0;
    long const kSamplesPerBeat = (sampleRate * 60) / kBpm;
    for (int i = 0; i < 16; ++i) {
        double noteAudioTime = noteBeatTime * (60.0 / kBpm);
        long noteTickTime = (long) (noteAudioTime * sampleRate);

        Event e;
        e.type = EventType::NoteOn;
        // e.timeInTicks = kSamplesPerBeat*i;
        e.timeInTicks = noteTickTime;
        e.midiNote = 69 + i;
        queue->push(e);

        e.type = EventType::NoteOff;
        // e.timeInTicks = kSamplesPerBeat*i + (kSamplesPerBeat / 2);
        e.timeInTicks = noteTickTime + (kSamplesPerBeat / 2);
        queue->push(e);

        noteBeatTime += 1.0;
    }
}

PaError Init(Context& context, float* pcmBuffer, unsigned long pcmBufferLength)
{
    PaError err;
    
    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

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

void ProcessEventQueue(EventQueue* eventQueue, std::array<Event, 256>* pendingEvents, int& pendingEventCount, long tickTime) {
    for (; pendingEventCount < pendingEvents->size(); ++pendingEventCount) {
        Event *e = eventQueue->front();
        if (e == nullptr) {
            return;
        }
        (*pendingEvents)[pendingEventCount] = *e;
        // long offset = e->timeInTicks - tickTime;
        //printf("count: %d, offset: %ld\n", pendingEventCount, offset);
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
            // // remove this element by swapping the last element with this one.
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

// int ProcessEventQueue(EventQueue* eventQueue, unsigned long framesPerBuffer, double const timeSecs, PendingEventBuffer* pendingEvents) {
//     long tickTime = timeSecs * SAMPLE_RATE;
//     int pendingEventCount = 0;
//     for (int i = 0; i < framesPerBuffer; ++i) {
//         Event* e = eventQueue->front();
//         while (e != nullptr && tickTime >= e->timeInTicks) {
//             if (e->timeInTicks == tickTime) {
//                 PendingEvent& pending = (*pendingEvents)[pendingEventCount];
//                 pending._e = *e;
//                 pending._sampleIx = i;
//                 ++pendingEventCount;
//                 if (pendingEventCount >= pendingEvents->size()) {
//                     std::cout << "MAXED OUT PENDING EVENT BUFFER!" << std::endl;
//                     return pendingEvents->size();
//                 }
//             }
//             eventQueue->pop();
//             e = eventQueue->front();
//         }
//         if (e == nullptr) {
//             break;
//         }
//         ++tickTime;
//     }
//     return pendingEventCount;
// }

int PortAudioCallback(
    const void *inputBuffer, void *const outputBufferUntyped,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData) {

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    StateData* state = (StateData*)userData;
    double const timeSecs = timeInfo->currentTime;
    
    // zero out the output buffer first.
    memset(outputBufferUntyped, 0, NUM_OUTPUT_CHANNELS * framesPerBuffer * sizeof(float));

    // Figure out which events apply to this invocation of the callback, and which effects should handle them.
    // PendingEventBuffer pendingEvents;
    // int const eventCount = ProcessEventQueue(state->events, framesPerBuffer, timeSecs, &pendingEvents);
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

    synth::Process(
        &state->synth, eventsThisFrame, numEventsThisFrame, outputBuffer,
        NUM_OUTPUT_CHANNELS, framesPerBuffer, SAMPLE_RATE);
    
    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    const double kSecsPerCallback = (double) framesPerBuffer / SAMPLE_RATE;
    double callbackTimeSecs = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    // printf("%f\n", callbackTimeSecs / kSecsPerCallback);
    if (callbackTimeSecs / kSecsPerCallback > 0.9) {
        printf("Frame close to deadline: %f / %f", callbackTimeSecs, kSecsPerCallback);
    }
        
    return paContinue;
}

}  // namespace audio