#include "audio.h"

#include <array>
#include <chrono>
#include <vector>

#include "audio_util.h"
#include "synth.h"
#include "sound_bank.h"

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
    StateData& state, std::vector<synth::Patch> const& synthPatches, SoundBank const& soundBank,
    EventQueue* eventQueue, int sampleRate) {

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

    state.soundBank = &soundBank;

    state.events = eventQueue;

    state.pendingEvents.set_capacity(256);
}

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
    printf("Stream completed\n");
}

PaError Init(
    Context& context, std::vector<synth::Patch> const& synthPatches, SoundBank const& soundBank) {

    PaError err;

    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    InitStateData(context._state, synthPatches, soundBank, &context._eventQueue, SAMPLE_RATE);

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

void ProcessEventQueue(EventQueue* eventQueue, boost::circular_buffer<Event>* pendingEvents, long tickTime) {
    while (!pendingEvents->full()) {
        Event *e = eventQueue->front();
        if (e == nullptr) {
            break;
        }
        pendingEvents->push_back(*e);
        eventQueue->pop();
    }
    if (pendingEvents->full()) {
        std::cout << "WARNING: WE FILLED THE PENDING EVENT LIST" << std::endl;
    }

    std::stable_sort(pendingEvents->begin(), pendingEvents->end(),
        [](audio::Event const& a, audio::Event const& b) {
            return a.timeInTicks < b.timeInTicks;
        });
}

void PopEventsFromThisFrame(
    boost::circular_buffer<Event>* pendingEvents, double const currentTimeSecs, unsigned long samplesPerFrame) {
    unsigned long const frameStartTickTime = currentTimeSecs * SAMPLE_RATE;
    unsigned long const nextFrameStartTickTime = frameStartTickTime + samplesPerFrame;
    int lastProcessedEventIx = -1;
    for (int pendingEventIx = 0; pendingEventIx < pendingEvents->size(); ++pendingEventIx) {
        Event const& e = (*pendingEvents)[pendingEventIx];
        if (e.timeInTicks < nextFrameStartTickTime) {
            lastProcessedEventIx = pendingEventIx;
        } else {
            break;
        }
    }
    if (lastProcessedEventIx >= 0) {
        pendingEvents->erase_begin(lastProcessedEventIx + 1);
    }
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
    unsigned long samplesPerFrame,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData) {

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    gLastFrameSize = samplesPerFrame;

    StateData* state = (StateData*)userData;
    double const timeSecs = timeInfo->currentTime;
    // double const timeSecs = timeInfo->outputBufferDacTime;

    // DEBUG
    gTimeSinceLastCallback = timeSecs - gLastCallbackTime;
    gTotalTime += gTimeSinceLastCallback;
    long ticksSinceLastCallback = (long)(gTimeSinceLastCallback * SAMPLE_RATE);
    if (ticksSinceLastCallback != samplesPerFrame) {
        ++gNumDesyncs;
    }
    gDevSum += (int)(std::abs(ticksSinceLastCallback - (long)samplesPerFrame));
    ++gTotalCallbacks;
    gLastCallbackTime = timeSecs;
    // DEBUG

    // zero out the output buffer first.
    memset(outputBufferUntyped, 0, NUM_OUTPUT_CHANNELS * samplesPerFrame * sizeof(float));

    // Figure out which events apply to this invocation of the callback, and which effects should handle them.
    ProcessEventQueue(state->events, &state->pendingEvents, timeSecs * SAMPLE_RATE);

    float* outputBuffer = (float*) outputBufferUntyped;

    // PCM playback first
    int currentEventIx = 0;
    unsigned long frameStartTickTime = timeSecs * SAMPLE_RATE;
    for (int i = 0; i < samplesPerFrame; ++i) {
        // Handle events
        while (true) {
            if (currentEventIx >= state->pendingEvents.size()) {
                break;
            }
            Event const& e = state->pendingEvents[currentEventIx];
            if (e.timeInTicks > frameStartTickTime + i) {
                break;
            }
            switch (e.type) {
                case EventType::PlayPcm: {
                    if (e.pcmSoundIx >= state->soundBank->_sounds.size()) {
                        std::cout << "NO PCM SOUND FOR NOTE " << e.pcmSoundIx << std::endl;
                        break;
                    }
                    PcmSound const& sound = state->soundBank->_sounds[e.pcmSoundIx];
                    if (sound._buffer == nullptr) {
                        std::cout << "PCM SOUND AT NOTE " << e.pcmSoundIx << " IS NULL" << std::endl;
                        break;
                    }
                    // Find free voice. Also check that there isn't another voice playing the exact same sample at the same time.
                    int voiceIx = -1;
                    bool foundCopy = false;
                    for (int i = 0; i < state->pcmVoices.size(); ++i) {
                        PcmVoice const& pcmVoice = state->pcmVoices[i];
                        if (pcmVoice._soundBufferIx < 0) {
                            voiceIx = i;
                        } else {
                            if (pcmVoice._soundIx == e.pcmSoundIx && pcmVoice._soundBufferIx <= 0) {
                                voiceIx = -1;
                                foundCopy = true;
                                break;
                            }
                        } 
                    }
                    if (foundCopy) {
                        break;
                    }
                    if (voiceIx < 0) {
                        std::cout << "NO MORE PCM VOICES" << std::endl;
                        break;
                    } else {
                        state->pcmVoices[voiceIx]._soundIx = e.pcmSoundIx;
                        state->pcmVoices[voiceIx]._soundBufferIx = 0;
                        state->pcmVoices[voiceIx]._gain = e.pcmVelocity;
                        state->pcmVoices[voiceIx]._loop = e.loop;
                    }

                    break;
                }
                case EventType::StopPcm: {
                    // Stop all voices currently playing the given sound.
                    for (int i = 0, n = state->pcmVoices.size(); i < n; ++i) {
                        PcmVoice& voice = state->pcmVoices[i];
                        if (voice._soundIx == e.pcmSoundIx) {
                            voice._soundBufferIx = -1;
                            voice._soundIx = -1;
                        }
                    }
                    break;
                }
                case EventType::AllNotesOff: {
                    // Stop all voices, period.
                    for (int i = 0, n = state->pcmVoices.size(); i < n; ++i) {
                        PcmVoice& voice = state->pcmVoices[i];
                        voice._soundBufferIx = -1;
                        voice._soundIx = -1;
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            ++currentEventIx;
        }

        float v = 0.0f;
        for (int voiceIx = 0; voiceIx < state->pcmVoices.size(); ++voiceIx) {
            PcmVoice& voice = state->pcmVoices[voiceIx];
            if (voice._soundIx < 0) {
                assert(voice._soundBufferIx < 0);
                continue;
            }
            PcmSound const& sound = state->soundBank->_sounds[voice._soundIx];
            if (sound._buffer == nullptr) {
                std::cout << "VOICE TRYING TO PLAY NULL BUFFER" << std::endl;
                continue;
            }
            assert(voice._soundBufferIx >= 0);
            assert(voice._soundBufferIx < sound._bufferLength);
            v += sound._buffer[voice._soundBufferIx] * voice._gain;
            ++voice._soundBufferIx;
            if (voice._soundBufferIx >= sound._bufferLength) {
                if (voice._loop) {
                    voice._soundBufferIx = 0;
                } else {
                    voice._soundBufferIx = -1;
                    voice._soundIx = -1;
                }                
            }
        }

        for (int channelIx = 0; channelIx < NUM_OUTPUT_CHANNELS; ++channelIx) {
            *outputBuffer++ += v;
        }
    }

    outputBuffer = (float*) outputBufferUntyped;

    for (synth::StateData& s : state->synths) {
        synth::Process(
            &s, state->pendingEvents, outputBuffer,
            NUM_OUTPUT_CHANNELS, samplesPerFrame, SAMPLE_RATE, frameStartTickTime);
    }

    // Clear out events from this frame.
    PopEventsFromThisFrame(&(state->pendingEvents), timeSecs, samplesPerFrame);

    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    const double kSecsPerCallback = (double) samplesPerFrame / SAMPLE_RATE;
    double callbackTimeSecs = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    if (callbackTimeSecs / kSecsPerCallback > 0.9) {
        printf("Frame close to deadline: %f / %f\n", callbackTimeSecs, kSecsPerCallback);
    }

    return paContinue;
}

}  // namespace audio
