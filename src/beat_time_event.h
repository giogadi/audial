#pragma once

#include <vector>

#include "audio_util.h"
#include "beat_clock.h"

class SoundBank;

struct BeatTimeEvent {
    audio::Event _e;  // timeInTicks is ignored.
    double _beatTime = 0.0;

    audio::Event ToTickTimeEvent(BeatClock const& beatClock) const {
        audio::Event e = _e;
        e.timeInTicks = beatClock.BeatTimeToTickTime(_beatTime);
        return e;
    }
    void FromTickTimeEvent(audio::Event const& e, BeatClock const& beatClock) {
        _e = e;
        _beatTime = beatClock.TickTimeToBeatTime(e.timeInTicks);
    }

    void MakeTimeRelativeTo(double beatTimeReference) {
        _beatTime = _beatTime - beatTimeReference;
    }

    void Save(serial::Ptree pt) const {
        _e.Save(pt);
        pt.PutDouble("beat_time", _beatTime);
    }
    void Load(serial::Ptree pt) {
        _e.Load(pt);
        _beatTime = pt.GetDouble("beat_time");
    }
};

inline audio::Event BeatTimeToTickTimeEvent(BeatTimeEvent const& b_e, BeatClock const& beatClock) {
    audio::Event e = b_e._e;
    e.timeInTicks = beatClock.BeatTimeToTickTime(b_e._beatTime);
    return e;
}

void ReadBeatEventsFromScript(
    std::vector<BeatTimeEvent>& events, SoundBank const& soundBank, char const* scriptBuf, int scriptBufLength);
void WriteBeatEventsToScript(
    std::vector<BeatTimeEvent> const& events, SoundBank const& soundBank, char* scriptBuf, int scriptBufLength);
// Find the time of the next instance of the given denomination, and generate an
// event that is at the given beat time offset from that denom.
audio::Event GetEventAtBeatOffsetFromNextDenom(
    double denom, BeatTimeEvent const& b_e, BeatClock const& beatClock, double slack = 0.0);