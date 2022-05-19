#pragma once

#include "SPSCQueue.h"
#include <array>

#include "enums/audio_EventType.h"
#include "enums/audio_SynthParamType.h"

namespace audio {

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