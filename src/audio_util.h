#pragma once

#include "SPSCQueue.h"
#include <array>

namespace audio {

enum class EventType {
    None, NoteOn, NoteOff, PlayPcm
};

struct Event {
    EventType type;
    int channel = -1;
    long timeInTicks = 0;
    int midiNote = 0;
};

static inline int const kEventQueueLength = 64;
typedef rigtorp::SPSCQueue<Event> EventQueue;

struct FrameEvent {
    Event _e;
    int _sampleIx;
};

// typedef std::array<PendingEvent, 256> PendingEventBuffer;
typedef std::array<FrameEvent, 256> EventsThisFrame;

}  // namespace audio