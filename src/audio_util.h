#pragma once

#include "SPSCQueue.h"
#include <array>

#include "enums/audio_EventType.h"
#include "enums/audio_SynthParamType.h"

namespace audio {

struct Event {
    Event() {
        type = EventType::None;
        channel = 0;
        timeInTicks = 0;
        midiNote = 0;
        velocity = 1.f;
    }
    EventType type;
    int channel;
    long timeInTicks;
    union {
        // If type is pcm, midiNote is the index of the sound to play.
        struct {
            int midiNote;
            float velocity;
        };
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