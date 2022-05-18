#pragma once

#include "SPSCQueue.h"
#include <array>

namespace audio {

#define M_AUDIO_EVENT_TYPES \
    X(None) \
    X(NoteOn) \
    X(NoteOff) \
    X(SynthParam) \
    X(PlayPcm)
enum class EventType : int {
#   define X(a) a,
    M_AUDIO_EVENT_TYPES
#   undef X
    Count
};
char const* EventTypeToString(EventType e);
EventType StringToEventType(char const* s);

#define M_AUDIO_SYNTH_PARAM_TYPES \
    X(Gain) \
    X(Cutoff) \
    X(Peak) \
    X(PitchLFOGain) \
    X(PitchLFOFreq) \
    X(CutoffLFOGain) \
    X(CutoffLFOFreq) \
    X(AmpEnvAttack) \
    X(AmpEnvDecay) \
    X(AmpEnvSustain) \
    X(AmpEnvRelease) \
    X(CutoffEnvGain) \
    X(CutoffEnvAttack) \
    X(CutoffEnvDecay) \
    X(CutoffEnvSustain) \
    X(CutoffEnvRelease)
enum class SynthParamType : int {
#   define X(a) a,
    M_AUDIO_SYNTH_PARAM_TYPES
#   undef X
    Count
};
char const* SynthParamTypeToString(SynthParamType e);
SynthParamType StringToSynthParamType(char const* s);

struct Event {
    EventType type;
    int channel = -1;
    long timeInTicks = 0;
    union {
        int midiNote = 0;
        struct {
            // valid under SynthParam type
            SynthParamType param;
            double newParamValue;
        };
    };
};

static inline int const kEventQueueLength = 64;
typedef rigtorp::SPSCQueue<Event> EventQueue;

struct FrameEvent {
    Event _e;
    int _sampleIx;
};

typedef std::array<FrameEvent, 256> EventsThisFrame;

}  // namespace audio