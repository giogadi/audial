#pragma once

#include "audio_util.h"
#include "beat_clock.h"

struct BeatTimeEvent {
    audio::Event _e;  // timeInTicks is ignored.
    double _beatTime = 0.0;

    void Save(ptree& pt) const {
        _e.Save(pt);
        pt.put("beat_time", _beatTime);
    }
    void Load(ptree const& pt) {
        _e.Load(pt);
        _beatTime = pt.get<double>("beat_time");
    }
};

inline audio::Event BeatTimeToTickTimeEvent(BeatTimeEvent const& b_e, BeatClock const& beatClock) {
    audio::Event e = b_e._e;
    e.timeInTicks = beatClock.BeatTimeToTickTime(b_e._beatTime);
    return e;
}