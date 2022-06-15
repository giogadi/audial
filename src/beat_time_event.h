#pragma once

#include "audio_util.h"
#include "beat_clock.h"

struct BeatTimeEvent {
    audio::Event _e;  // timeInTicks is ignored.
    double _beatTime = 0.0;
};

inline audio::Event BeatTimeToTickTimeEvent(BeatTimeEvent const& b_e, BeatClock const& beatClock) {
    audio::Event e = b_e._e;
    e.timeInTicks = beatClock.BeatTimeToTickTime(b_e._beatTime);
    return e;
}