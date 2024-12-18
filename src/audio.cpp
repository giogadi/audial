#include "audio.h"

#include <array>
#include <chrono>
#include <vector>

#include "SPSCQueue.h"

#include "samplerate.h"

#include "features.h"
#include "util.h"
#include "audio_util.h"
#include "synth.h"
#include "sound_bank.h"

using std::chrono::high_resolution_clock;

#define NUM_OUTPUT_CHANNELS (1)
#define INTERNAL_SR (48000)

namespace audio {

namespace {

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

float *gInternalBuffer = nullptr;
std::size_t gInternalBufferSize = 0;
int gLeftoverInputFrames = 0;
int gLeftoverInputFramesStartIx = -1;

SRC_STATE *gSrcState = nullptr;

typedef rigtorp::SPSCQueue<Event> EventQueue;
int constexpr kEventQueueLength = 1024;

EventQueue sEventQueue(kEventQueueLength);

} // namespace

bool AddEvent(Event const &e) {
    bool success = sEventQueue.try_push(e);
    if (!success) {
        // TODO: maybe use serialize to get a string of the event
        std::cout << "Failed to add event to audio queue:" << std::endl;
    }
    return success;
}

void InitStateData(StateData& state, SoundBank const& soundBank, int outputSampleRate, int framesPerBuffer) {

    if (!gInternalBuffer) {
        // TODO: can we reduce this size safely?
        gInternalBufferSize = 3 * framesPerBuffer * NUM_OUTPUT_CHANNELS * sizeof(float);
        gInternalBuffer = new float[gInternalBufferSize];
    }
    gLeftoverInputFrames = 0;
    gLeftoverInputFramesStartIx = -1;

    
    state.outputSampleRate = outputSampleRate;

    for (int i = 0; i < state.synths.size(); ++i) {
        synth::StateData& s = state.synths[i];
        synth::InitStateData(s, /*channel=*/i, INTERNAL_SR, framesPerBuffer, NUM_OUTPUT_CHANNELS);
    }

    state.soundBank = &soundBank;

    int constexpr kMaxSize = 1024;
    delete[] state.pendingEvents.entries;
    state.pendingEvents = PendingEventHeap();
    state.pendingEvents.entries = new HeapEntry[kMaxSize];
    state.pendingEvents.maxSize = kMaxSize;

    state._bufferFrameCount = framesPerBuffer;
    state._recentBuffer = new float[framesPerBuffer];

    int srcErr = 0;
    gSrcState = src_new(SRC_SINC_FASTEST, NUM_OUTPUT_CHANNELS, &srcErr);
    if (srcErr) {
        printf("Audio error in creating SRC state: %s\n", src_strerror(srcErr));
    }
}
void DestroyStateData(StateData& state) {
    for (synth::StateData& synth : state.synths) {
        synth::DestroyStateData(synth);
    }

    delete[] state._recentBuffer;
    delete[] state.pendingEvents.entries;
    if (gInternalBuffer) {
        delete[] gInternalBuffer;
    }
    state.pendingEvents = PendingEventHeap();

    src_delete(gSrcState);
    gSrcState = nullptr;
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

void FillBuffer(
    StateData *state, float *const outputBufferIn, int framesPerBuffer, int sampleRate) {
     
    // zero out the output buffer first.
    memset(outputBufferIn, 0, NUM_OUTPUT_CHANNELS * framesPerBuffer * sizeof(float));

    // Figure out which events apply to this invocation of the callback, and which effects should handle them.
    ProcessEventQueue(&sEventQueue, state->_bufferCounter, sampleRate, framesPerBuffer, &state->pendingEvents);

    int constexpr kMaxSize = 1024;
    static PendingEvent eventsThisBuffer[kMaxSize];
    int eventsThisBufferCount = 0;
    while (state->pendingEvents.size > 0) {
        PendingEvent e = *HeapPeek(&state->pendingEvents);
        if (state->_bufferCounter < e._runBufferCounter) {
            break;
        }
        if (eventsThisBufferCount >= kMaxSize) {
            printf("AUDIO PROBLEM: EventsThisBuffer not big enough!\n");
            break;
        }
        eventsThisBuffer[eventsThisBufferCount++] = HeapPop(&state->pendingEvents);
    }

    // PCM playback first
    float *outputBuffer = outputBufferIn;
    int currentEventIx = 0;
    for (int i = 0; i < framesPerBuffer; ++i) {
        // Handle events
        for (int eventIx = 0; eventIx < eventsThisBufferCount; ++eventIx) {
            Event const& e = eventsThisBuffer[eventIx]._e; 
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


    for (synth::StateData& s : state->synths) {
        synth::Process(
            &s, eventsThisBuffer, eventsThisBufferCount, outputBufferIn,
            NUM_OUTPUT_CHANNELS, framesPerBuffer, sampleRate, state->_bufferCounter);
    }

    if (state->_finalGain != 1.f) {
        for (int i = 0, n = framesPerBuffer * NUM_OUTPUT_CHANNELS; i < n; ++i) {
            outputBufferIn[i] *= state->_finalGain;
        }
    }
 
    ++state->_bufferCounter;
}

void FillBufferAndResample(StateData *state, float *outputBuffer, unsigned long framesPerBuffer) {
    // TODO this should only be computed once way at the beginning, right?
    float numInputFramesExactFrac = (float)framesPerBuffer * (float) INTERNAL_SR / (float) state->outputSampleRate;
    int numInputFramesNeededExact = (int) ceilf(numInputFramesExactFrac);
    int minNumInputFrames = numInputFramesNeededExact; 
    if (minNumInputFrames > framesPerBuffer) {
        minNumInputFrames = framesPerBuffer * 2;
        if (minNumInputFrames < numInputFramesNeededExact) {
            printf("ERROR not generating enough input frames: %d < %d\n", minNumInputFrames, numInputFramesNeededExact);
        }
    }

    int numInputFramesGenerated = 0; 
    if (gLeftoverInputFrames > 0) { 
        float *leftoversStart = &gInternalBuffer[gLeftoverInputFramesStartIx * NUM_OUTPUT_CHANNELS];
        size_t leftoversSize = gLeftoverInputFrames * sizeof(float) * NUM_OUTPUT_CHANNELS;
        assert((char*)leftoversStart + leftoversSize <= (char*)gInternalBuffer + gInternalBufferSize);
        memmove(gInternalBuffer, leftoversStart, leftoversSize);
        numInputFramesGenerated += gLeftoverInputFrames;
    }

    if (numInputFramesGenerated < minNumInputFrames) {
        float *out = gInternalBuffer;
        out += gLeftoverInputFrames * NUM_OUTPUT_CHANNELS;
        assert((char*)out + framesPerBuffer * NUM_OUTPUT_CHANNELS * sizeof(float) <= (char*)gInternalBuffer + gInternalBufferSize);
        FillBuffer(state, out, framesPerBuffer, INTERNAL_SR);
        numInputFramesGenerated += framesPerBuffer;
        if (numInputFramesGenerated < minNumInputFrames) {
            out += framesPerBuffer * NUM_OUTPUT_CHANNELS;
            assert((char*)out + framesPerBuffer * NUM_OUTPUT_CHANNELS * sizeof(float) <= (char*)gInternalBuffer + gInternalBufferSize);
            FillBuffer(state, out, framesPerBuffer, INTERNAL_SR);
            numInputFramesGenerated += framesPerBuffer;
            if (numInputFramesGenerated < minNumInputFrames) {
                printf("audio.cpp: okay wtf is this? left:%d, fpb:%lu nif:%d\n", gLeftoverInputFrames, framesPerBuffer, minNumInputFrames);
            }
        }
    } 

#if 1    
    // RESAMPLE using numInputFramesGenerated from gInternalBuffer!
    SRC_DATA resampleData = {0};
    resampleData.data_in = gInternalBuffer;
    resampleData.data_out = outputBuffer;
    resampleData.input_frames = numInputFramesGenerated;
    resampleData.output_frames = (long)framesPerBuffer;
    resampleData.src_ratio = (double)state->outputSampleRate / (double)INTERNAL_SR;
    int srcErr = src_process(gSrcState, &resampleData);
    if (srcErr) {
        printf("Audio error: src_process error: %s\n", src_strerror(srcErr));
    }
    gLeftoverInputFrames = resampleData.input_frames - resampleData.input_frames_used;
    gLeftoverInputFramesStartIx = resampleData.input_frames_used;
    if (gLeftoverInputFrames < 0) {
        printf("negative input frames?!?!?! input: %ld used: %ld\n", resampleData.input_frames, resampleData.input_frames_used);
    }
    if (resampleData.output_frames_gen != framesPerBuffer) {
        printf("undergen: %ld %ld\n", resampleData.input_frames_used, resampleData.output_frames_gen);
        memset(outputBuffer, 0, sizeof(float) * NUM_OUTPUT_CHANNELS * framesPerBuffer);
    }
#else
    memcpy(outputBuffer, gInternalBuffer, sizeof(float) * NUM_OUTPUT_CHANNELS * framesPerBuffer);
    gLeftoverInputFrames = numInputFramesGenerated - framesPerBuffer;
    gLeftoverInputFramesStartIx = framesPerBuffer;
#endif
}

void AudioCallback(
    float const* inputBuffer, float * const outputBuffer, unsigned long framesPerBuffer, StateData *state) {

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    if (state->outputSampleRate == INTERNAL_SR) {
        FillBuffer(state, outputBuffer, framesPerBuffer, INTERNAL_SR);
    } else {
        FillBufferAndResample(state, outputBuffer, framesPerBuffer);
    } 
     
#if COMPUTE_FFT
    if (state->_recentBufferMutex.try_lock()) {
        for (int i = 0; i < framesPerBuffer; ++i) {
            state->_recentBuffer[i] = outputBuffer[i];
        }
        state->_recentBufferMutex.unlock();
    }
#endif
 
    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    const double kSecsPerCallback = static_cast<double>(framesPerBuffer) / state->outputSampleRate;
    double callbackTimeSecs = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    if (callbackTimeSecs / kSecsPerCallback > 0.9) {
        printf("Frame close to deadline: %f / %f\n", callbackTimeSecs, kSecsPerCallback);
    }    
}

int InternalSampleRate() {
    return INTERNAL_SR;
}

int NumOutputChannels() {
    return NUM_OUTPUT_CHANNELS;
}

}  // namespace audio
