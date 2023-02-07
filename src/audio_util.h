#pragma once

#include <array>

#include "SPSCQueue.h"

#include "enums/audio_EventType.h"
#include "enums/audio_SynthParamType.h"
#include "serial.h"

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
        struct {
            int midiNote;
            float velocity;
        };
        // valid under pcm type
        struct {
            int pcmSoundIx;
            float pcmVelocity;
            bool loop;
        };
        struct {
            // valid under SynthParam type
            SynthParamType param;
            long paramChangeTime;  // if 0, param gets changed instantly.
            float newParamValue;
            int newParamValueInt;
        };
        struct {
            // valid under SetGain
            float newGain;
        };
    };

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
};

static inline int const kEventQueueLength = 64;
typedef rigtorp::SPSCQueue<Event> EventQueue;

struct FrameEvent {
    Event _e;
    int _sampleIx;
};

typedef std::array<FrameEvent, 256> EventsThisFrame;

struct PcmSound {
    float* _buffer = nullptr;
    uint64_t _bufferLength = 0;
};

}  // namespace audio
