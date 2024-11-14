#include "audio.h"

#include <array>
#include <chrono>
#include <vector>

#ifdef _WIN32
#include "portaudio/include/pa_win_wasapi.h"
#endif

#include "features.h"
#include "util.h"
#include "audio_util.h"
#include "synth.h"
#include "sound_bank.h"

using std::chrono::high_resolution_clock;

#define NUM_OUTPUT_CHANNELS (1)
#define FRAMES_PER_BUFFER  (512)

namespace audio {

namespace {

void OnPortAudioError(PaError const& err) {
    Pa_Terminate();
    std::cout << "An error occured while using the portaudio stream" << std::endl;
    std::cout << "Error number: " << err << std::endl;
    std::cout << "Error message: " << Pa_GetErrorText(err) << std::endl;
}

bool HeapLess(HeapEntry const &lhs, HeapEntry const &rhs) {
    if (lhs.key == rhs.key) {
        return lhs.counter < rhs.counter;
    }
    return lhs.key < rhs.key;
}

int HeapParent(int ix) {
    return (ix - 1) / 2;
}

int HeapLeftChild(int ix) {
    return 2*ix + 1;
}

void HeapInsert(PendingEventHeap *heap, int key, PendingEvent const& event) {
    if (heap->size >= heap->maxSize) {
        return;
    }
    int currentIx = heap->size;
    ++(heap->size);
    heap->entries[currentIx].e = event;
    heap->entries[currentIx].key = key;
    heap->entries[currentIx].counter = heap->counter++;

    while (currentIx > 0) {
        int parentIx = HeapParent(currentIx);
        HeapEntry& parent = heap->entries[parentIx];
        if (HeapLess(heap->entries[currentIx], parent)) {
            HeapEntry e = heap->entries[currentIx];
            heap->entries[currentIx] = heap->entries[parentIx];
            heap->entries[parentIx] = e;
            currentIx = parentIx;
        } else {
            break;
        }
    }    
}

PendingEvent HeapPop(PendingEventHeap *heap) {
    PendingEvent output;
    if (heap->size <= 0) {
        printf("AUDIO ERROR: tried to pop off empty heap!\n");
        return output;
    }

    output = heap->entries[0].e;
    heap->entries[0] = heap->entries[heap->size - 1];
    --(heap->size);

    int currentIx = 0;
    int leftChildIx = 1;
    while (leftChildIx < heap->size) {
        int minChildIx = leftChildIx;
        if (leftChildIx + 1 < heap->size) {
            int rightChildIx = leftChildIx + 1;
            if (HeapLess(heap->entries[rightChildIx], heap->entries[leftChildIx])) {
                minChildIx = rightChildIx;
            }            
        }
        if (HeapLess(heap->entries[minChildIx], heap->entries[currentIx])) {
            HeapEntry e = heap->entries[currentIx];
            heap->entries[currentIx] = heap->entries[minChildIx];
            heap->entries[minChildIx] = e;            
            currentIx = minChildIx;
            leftChildIx = HeapLeftChild(minChildIx);
        } else {
            break;
        }
    }

    return output;
} 

PendingEvent *HeapPeek(PendingEventHeap *heap) {
    if (heap->size <= 0) {
        printf("AUDIO ERROR: tried to peek empty heap!\n");
        return nullptr;
    }
    return &heap->entries[0].e;
}

} // namespace

void InitStateData(
    StateData& state, SoundBank const& soundBank,
    EventQueue* eventQueue, int sampleRate) {

    state.sampleRate = sampleRate;

    for (int i = 0; i < state.synths.size(); ++i) {
        synth::StateData& s = state.synths[i];
        synth::InitStateData(s, /*channel=*/i, sampleRate, FRAMES_PER_BUFFER, NUM_OUTPUT_CHANNELS);
    }

    state.soundBank = &soundBank;

    state.events = eventQueue;

    int constexpr kMaxSize = 1024;
    delete[] state.pendingEvents.entries;
    state.pendingEvents = PendingEventHeap();
    state.pendingEvents.entries = new HeapEntry[kMaxSize];
    state.pendingEvents.maxSize = kMaxSize;

    state._bufferFrameCount = FRAMES_PER_BUFFER;
    state._recentBuffer = new float[FRAMES_PER_BUFFER];
}
void DestroyStateData(StateData& state) {
    for (synth::StateData& synth : state.synths) {
        synth::DestroyStateData(synth);
    }

    delete[] state._recentBuffer;
    delete[] state.pendingEvents.entries;
    state.pendingEvents = PendingEventHeap();
}

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
}

PaError Init(
    Context& context, SoundBank const& soundBank) {
    int const kPreferredSampleRate = 48000;

    PaError err;

    err = Pa_Initialize();
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    context._sampleRate = kPreferredSampleRate;
    std::vector<PaError> errors;
#ifdef _WIN32
    PaHostApiIndex const hostApiCount = Pa_GetHostApiCount();
    PaHostApiInfo const* hostApiInfo = 0;
    for (PaHostApiIndex hostApiIx = 0; hostApiIx < hostApiCount; ++hostApiIx) {
        PaHostApiInfo const *thisHost = Pa_GetHostApiInfo(hostApiIx);
        if (thisHost->type == paWASAPI) {
            hostApiInfo = thisHost;
            break;
        }
    }
    if (!hostApiInfo) {
        printf("WARNING: could not find a WASAPI audio host. You may have degraded/delayed audio performance.\n");        
        hostApiInfo = Pa_GetHostApiInfo(Pa_GetDefaultHostApi());
        printf("Using audio host API \"%s\"\n", hostApiInfo->name);
    }
    PaDeviceIndex deviceIx = hostApiInfo->defaultOutputDevice;
    context._outputParameters.device = deviceIx;
    PaDeviceInfo const* pDeviceInfo = Pa_GetDeviceInfo(deviceIx);
    // Check if our preferred sample rate is supported. If not, just use the device's default sample rate.
    PaStreamParameters params = {0};
    params.device = context._outputParameters.device;
    params.channelCount = NUM_OUTPUT_CHANNELS;
    params.sampleFormat = paFloat32;
    double sampleRate = kPreferredSampleRate;
    err = Pa_IsFormatSupported(0, &params, sampleRate);
    if (err) {
        // TODO: show previous error        
        sampleRate = pDeviceInfo->defaultSampleRate;
        err = Pa_IsFormatSupported(0, &params, sampleRate);
        if (err) {
            printf("Default device does not support required audio formats.\n");
        }
    }
    context._sampleRate = (int)sampleRate;
    context._outputParameters.device = deviceIx;
    printf("Selected audio device: %s\n", pDeviceInfo->name);
    printf("Device sample rate: %d\n", (int)sampleRate);
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

void ProcessEventQueue(EventQueue* eventQueue, int64_t currentBufferCounter, double sampleRate, int bufferSize, PendingEventHeap* pendingEvents) {
    double const secsToBuffers = sampleRate / static_cast<double>(bufferSize);
    while (pendingEvents->size < pendingEvents->maxSize) {
        Event *e = eventQueue->front();
        if (e == nullptr) {
            break;
        }
        PendingEvent p_e;
        p_e._e = *e;
        int64_t delayInBuffers = static_cast<int64_t>(e->delaySecs * secsToBuffers);
        p_e._runBufferCounter = currentBufferCounter + delayInBuffers;
        HeapInsert(pendingEvents, p_e._runBufferCounter, p_e);
        eventQueue->pop();
    }
    if (pendingEvents->size >= pendingEvents->maxSize) {
        std::cout << "WARNING: WE FILLED THE PENDING EVENT LIST" << std::endl;
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

    int constexpr kMaxSize = 1024;
    PendingEvent eventsThisFrame[kMaxSize]; // TODO UGH
    int eventsThisFrameCount = 0;
    while (state->pendingEvents.size > 0) {
        PendingEvent e = *HeapPeek(&state->pendingEvents);
        if (state->_bufferCounter < e._runBufferCounter) {
            break;
        }
        if (eventsThisFrameCount >= kMaxSize) {
            printf("AUDIO PROBLEM: EventsThisFrame not big enough!\n");
            break;
        }
        eventsThisFrame[eventsThisFrameCount++] = HeapPop(&state->pendingEvents);
    }

    float* outputBuffer = (float*) outputBufferUntyped;

    // PCM playback first
    int currentEventIx = 0;
    for (int i = 0; i < samplesPerFrame; ++i) {
        // Handle events
        for (int eventIx = 0; eventIx < eventsThisFrameCount; ++eventIx) {
            Event const& e = eventsThisFrame[eventIx]._e; 
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
            &s, eventsThisFrame, eventsThisFrameCount, outputBuffer,
            NUM_OUTPUT_CHANNELS, samplesPerFrame, state->sampleRate, state->_bufferCounter);
    }

    if (state->_finalGain != 1.f) {
        for (int i = 0, n = samplesPerFrame * NUM_OUTPUT_CHANNELS; i < n; ++i) {
            outputBuffer[i] *= state->_finalGain;
        }
    }

#if COMPUTE_FFT
    if (state->_recentBufferMutex.try_lock()) {
        for (int i = 0; i < samplesPerFrame; ++i) {
            state->_recentBuffer[i] = outputBuffer[i];
        }
        state->_recentBufferMutex.unlock();
    }
#endif
 
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
