#pragma once

#include "SPSCQueue.h"
#include <array>

namespace audio {

enum class EventType {
    None, NoteOn, NoteOff, SynthParam, PlayPcm
};
enum class ParamType : int {
    Gain, Cutoff, Peak, PitchLFOGain, PitchLFOFreq, CutoffLFOGain, CutoffLFOFreq, AmpEnvAttack, AmpEnvDecay, AmpEnvSustain, AmpEnvRelease,
    NumParams
};

struct Event {
    EventType type;
    int channel = -1;
    long timeInTicks = 0;
    union {
        int midiNote = 0;
        struct {
            // valid under SynthParam type
            ParamType param;
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