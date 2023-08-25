#pragma once

#include <vector>

#include "audio_util.h"
#include "beat_clock.h"

class SoundBank;

struct BeatTimeEvent {
    audio::Event _e;  // timeInTicks is ignored.
    double _beatTime = 0.0;

    audio::Event ToEvent(BeatClock const& beatClock) const {
        audio::Event e = _e;
        e.delaySecs = beatClock.BeatTimeToSecs(_beatTime);
        return e;
    }
    void FromEvent(audio::Event const& e, BeatClock const& beatClock) {
        _e = e;
        _beatTime = beatClock.SecsToBeatTime(e.delaySecs);
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
    bool ImGui();
};

inline audio::Event BeatTimeEventToEvent(BeatTimeEvent const& b_e, BeatClock const& beatClock) {
    audio::Event e = b_e.ToEvent(beatClock);
    return e;
}

// Returns true if soundBank name was given
bool ReadBeatEventFromTextLineNoSoundLookup(std::istream& lineStream, BeatTimeEvent& b_e, std::string& soundBankName);

void ReadBeatEventFromTextLine(SoundBank const& soundBank, std::istream& lineStream, BeatTimeEvent& b_e);
void ReadBeatEventsFromScript(
    std::vector<BeatTimeEvent>& events, SoundBank const& soundBank, char const* scriptBuf, int scriptBufLength);
void WriteBeatEventsToScript(
    std::vector<BeatTimeEvent> const& events, SoundBank const& soundBank, char* scriptBuf, int scriptBufLength);
// Find the time of the next instance of the given denomination, and generate an
// event that is at the given beat time offset from that denom.
audio::Event GetEventAtBeatOffsetFromNextDenom(
    double denom, BeatTimeEvent const& b_e, BeatClock const& beatClock, double slack = 0.0);
