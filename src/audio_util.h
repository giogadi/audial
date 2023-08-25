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
        delaySecs = 0;
        midiNote = 0;
        velocity = 1.f;
        noteOnId = 0;
    }
    EventType type;
    int channel;
    double delaySecs;
    union {
        struct {
            int midiNote;
            float velocity;
            // If non-zero, then note-ons will be tagged with this number. Then,
            // this note will only get turned off from note-offs with this same
            // ID (or note-offs with a zero ID).
            int noteOnId;
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
            double paramChangeTimeSecs;  // if 0, param gets changed instantly.
            float newParamValue;
        };
        struct {
            // valid under SetGain
            float newGain;
        };
    };

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
};

static inline int const kEventQueueLength = 1024;
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

struct PendingEvent {
    Event _e;
    int64_t _runBufferCounter = 0;
};

}  // namespace audio
