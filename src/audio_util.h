#pragma once

#include <array>

#include "SPSCQueue.h"
#include "boost/property_tree/ptree.hpp"

#include "enums/audio_EventType.h"
#include "enums/audio_SynthParamType.h"
#include "serial.h"

using boost::property_tree::ptree;

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
            int newParamValueInt;
        };
    };

    void Save(ptree& pt) const;
    void Save(serial::Ptree pt) const;
    void Load(ptree const& pt);
    void Load(serial::Ptree pt);
};

static inline int const kEventQueueLength = 64;
typedef rigtorp::SPSCQueue<Event> EventQueue;

struct FrameEvent {
    Event _e;
    int _sampleIx;
};

typedef std::array<FrameEvent, 256> EventsThisFrame;

}  // namespace audio