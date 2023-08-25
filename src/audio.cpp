#include "audio.h"

#include <array>
#include <chrono>
#include <vector>

#ifdef _WIN32
#include "portaudio/include/pa_win_wasapi.h"
#endif

#include "util.h"
#include "audio_util.h"
#include "synth.h"
#include "sound_bank.h"

using std::chrono::high_resolution_clock;

#define NUM_OUTPUT_CHANNELS (1)

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
    StateData& state, SoundBank const& soundBank,
    EventQueue* eventQueue, int sampleRate) {

    state.sampleRate = sampleRate;

    for (int i = 0; i < state.synths.size(); ++i) {
        synth::StateData& s = state.synths[i];
        synth::InitStateData(s, /*channel=*/i, FRAMES_PER_BUFFER, NUM_OUTPUT_CHANNELS);
    }

    state.soundBank = &soundBank;

    state.events = eventQueue;

    state.pendingEvents.set_capacity(1024);
}
void DestroyStateData(StateData& state) {
    for (synth::StateData& synth : state.synths) {
        synth::DestroyStateData(synth);
    }
}

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
}

PaError Init(
    Context& context, SoundBank const& soundBank) {
    int const kSupportedSampleRates[] = {
        48000, 44100
    };

    PaError err;

    err = Pa_Initialize();
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    void* streamInfo = nullptr;
    context._sampleRate = kSupportedSampleRates[0];
    std::vector<PaError> errors;
#ifdef _WIN32
    PaWasapiStreamInfo wasapiStreamInfo{};
    wasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
    wasapiStreamInfo.hostApiType = paWASAPI;
    wasapiStreamInfo.version = 1;
    // sWasapiStreamInfo.flags = paWinWasapiExclusive|paWinWasapiThreadPriority;
    // sWasapiStreamInfo.flags = paWinWasapiThreadPriority;
    wasapiStreamInfo.threadPriority = eThreadPriorityProAudio;
    streamInfo = &wasapiStreamInfo;

    context._outputParameters.device = paNoDevice;
    PaDeviceIndex numDevices = Pa_GetDeviceCount();   
    for (PaDeviceIndex deviceIx = 0; deviceIx < numDevices; ++deviceIx) {
        PaDeviceInfo const* pDeviceInfo = Pa_GetDeviceInfo(deviceIx);
        PaHostApiInfo const* pHostApiInfo = Pa_GetHostApiInfo(pDeviceInfo->hostApi);
        if (pHostApiInfo->type != paWASAPI) {
            err = paInvalidHostApi;
            continue;
        }
        if (deviceIx == pHostApiInfo->defaultOutputDevice) {
            printf("Default device sample rate: %f\n", pDeviceInfo->defaultSampleRate);
            printf("Default device max output channels: %d\n", pDeviceInfo->maxOutputChannels);
            context._outputParameters.device = deviceIx;

            PaStreamParameters testOutputParams{};
            testOutputParams.device = deviceIx;
            testOutputParams.channelCount = NUM_OUTPUT_CHANNELS;
            testOutputParams.sampleFormat = paFloat32;
            testOutputParams.hostApiSpecificStreamInfo = streamInfo;
            for (int sampleRateIx = 0, n = M_ARRAY_LEN(kSupportedSampleRates); sampleRateIx < n; ++sampleRateIx) {
                context._sampleRate = kSupportedSampleRates[sampleRateIx];
                err = Pa_IsFormatSupported(NULL, &testOutputParams, context._sampleRate);
                if (err == paNoError) {
                    break;
                } else {
                    errors.push_back(err);
                }
            }
            break;
        }
    }
#else
    context._outputParameters.device = Pa_GetDefaultOutputDevice();
#endif     

    if (err != paNoError) {
        printf("Audio error: default audio device not supported.\n");
        char const* finalErrMsg = Pa_GetErrorText(err);
        printf("Final error: %s\n", finalErrMsg);
        printf("Error list:\n");
        for (int i = 0; i < errors.size(); ++i) {
            char const* errMsg = Pa_GetErrorText(errors[i]);
            printf("Error %d: %s\n", i, errMsg);
        }
        return err;
    }
    
    InitStateData(context._state, soundBank, &context._eventQueue, context._sampleRate);

    context._outputParameters.hostApiSpecificStreamInfo = streamInfo;
    
    context._outputParameters.channelCount = NUM_OUTPUT_CHANNELS;
    context._outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    context._outputParameters.suggestedLatency = Pa_GetDeviceInfo( context._outputParameters.device )->defaultLowOutputLatency;    

    err = Pa_OpenStream(
              &context._stream,
              NULL, /* no input */
              &context._outputParameters,
              context._sampleRate,
              FRAMES_PER_BUFFER,
              paNoFlag,  // no flags (clipping is on)
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

    DestroyStateData(context._state);
    return paNoError;
}

void ProcessEventQueue(EventQueue* eventQueue, int64_t currentBufferCounter, double sampleRate, int bufferSize, boost::circular_buffer<PendingEvent>* pendingEvents) {
    double const secsToBuffers = sampleRate / static_cast<double>(bufferSize);
    while (!pendingEvents->full()) {
        Event *e = eventQueue->front();
        if (e == nullptr) {
            break;
        }
        PendingEvent p_e;
        p_e._e = *e;
        int64_t delayInBuffers = e->delaySecs * secsToBuffers;
        p_e._runBufferCounter = currentBufferCounter + delayInBuffers;
        pendingEvents->push_back(p_e);
        eventQueue->pop();
    }
    if (pendingEvents->full()) {
        std::cout << "WARNING: WE FILLED THE PENDING EVENT LIST" << std::endl;
    }

    std::stable_sort(pendingEvents->begin(), pendingEvents->end(),
        [](audio::PendingEvent const& a, audio::PendingEvent const& b) {
            return a._runBufferCounter < b._runBufferCounter;
        });
}

void PopEventsFromThisFrame(
    boost::circular_buffer<PendingEvent>* pendingEvents, int64_t currentBufferCounter) {
    int lastProcessedEventIx = -1;
    for (int pendingEventIx = 0, n = pendingEvents->size(); pendingEventIx < n; ++pendingEventIx) {
        PendingEvent const& p_e = (*pendingEvents)[pendingEventIx];
        if (p_e._runBufferCounter <= currentBufferCounter) {
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

double gAvgCallbackTime = 0.0;
double gMaxCallbackTime = 0.0;
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

    // DEBUG
    gTimeSinceLastCallback = timeSecs - gLastCallbackTime;
    gTotalTime += gTimeSinceLastCallback;
    long ticksSinceLastCallback = (long)(gTimeSinceLastCallback * state->sampleRate);
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
    ProcessEventQueue(state->events, state->_bufferCounter, state->sampleRate, samplesPerFrame, &state->pendingEvents);

    float* outputBuffer = (float*) outputBufferUntyped;

    // PCM playback first
    int currentEventIx = 0;
    for (int i = 0; i < samplesPerFrame; ++i) {
        // Handle events
        while (true) {
            if (currentEventIx >= state->pendingEvents.size()) {
                break;
            }
            PendingEvent const& p_e = state->pendingEvents[currentEventIx];
            if (state->_bufferCounter < p_e._runBufferCounter) {
                break;
            }
            Event const& e = p_e._e;
            switch (e.type) {
                case EventType::PlayPcm: {
                    if (e.pcmSoundIx >= state->soundBank->_sounds.size() || e.pcmSoundIx < 0) {
                        std::cout << "NO PCM SOUND FOR NOTE " << e.pcmSoundIx << std::endl;
                        break;
                    }
                    PcmSound const& sound = state->soundBank->_sounds[e.pcmSoundIx];
                    if (sound._buffer == nullptr) {
                        std::cout << "PCM SOUND AT NOTE " << e.pcmSoundIx << " IS NULL" << std::endl;
                        break;
                    }
                    // Find free voice. If this sample is already playing, or if
                    // the new sample is in the same exclusive group as the
                    // previous one, cancel the other one and play the new one.
                    int voiceIx = -1;
                    for (int i = 0; i < state->pcmVoices.size(); ++i) {
                        PcmVoice const& pcmVoice = state->pcmVoices[i];
                        if (pcmVoice._soundBufferIx < 0) {
                            if (voiceIx < 0) {
                                voiceIx = i;
                            }
                        } else if (pcmVoice._soundIx == e.pcmSoundIx) {
                            voiceIx = i;                            
                        } else {
                            int prevGroup = state->soundBank->_exclusiveGroups[pcmVoice._soundIx];
                            int newGroup = state->soundBank->_exclusiveGroups[e.pcmSoundIx];
                            if (prevGroup >= 0 && prevGroup == newGroup) {
                                voiceIx = i;
                            }
                        }
                        
                    }
                    if (voiceIx < 0) {
                        printf("NO MORE PCM VOICES!\n");
                        break;
                    }
                    state->pcmVoices[voiceIx]._soundIx = e.pcmSoundIx;
                    state->pcmVoices[voiceIx]._soundBufferIx = 0;
                    state->pcmVoices[voiceIx]._gain = e.pcmVelocity;
                    state->pcmVoices[voiceIx]._loop = e.loop;
                    
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
                case EventType::SetGain: {
                    state->_finalGain = e.newGain;
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
            NUM_OUTPUT_CHANNELS, samplesPerFrame, state->sampleRate, state->_bufferCounter);
    }

    if (state->_finalGain != 1.f) {
        for (int i = 0, n = samplesPerFrame * NUM_OUTPUT_CHANNELS; i < n; ++i) {
            outputBuffer[i] *= state->_finalGain;
        }
    }

    // Clear out events from this frame.
    PopEventsFromThisFrame(&(state->pendingEvents), state->_bufferCounter);

    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    const double kSecsPerCallback = static_cast<double>(samplesPerFrame) / state->sampleRate;
    double callbackTimeSecs = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    if (callbackTimeSecs / kSecsPerCallback > 0.9) {
        printf("Frame close to deadline: %f / %f\n", callbackTimeSecs, kSecsPerCallback);
    }

    //const int kCallbacksPerSec = (int) (1.0 / kSecsPerCallback);
    //if (gTotalCallbacks % kCallbacksPerSec == 0) {
    //    printf("(avg,max): %f, %f\n", gAvgCallbackTime / kSecsPerCallback, gMaxCallbackTime / kSecsPerCallback);
    //    gAvgCallbackTime = 0.0;
    //    gMaxCallbackTime = 0.0;
    //} else {
    //    gAvgCallbackTime += callbackTimeSecs / kCallbacksPerSec;
    //    gMaxCallbackTime = std::max(callbackTimeSecs, gMaxCallbackTime);
    //}

    ++state->_bufferCounter;

    return paContinue;
}

}  // namespace audio
