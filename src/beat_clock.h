#pragma once

#include <cmath>
#include <cstdint>
#include "game_manager.h"

class BeatClock {
public:
    void Init(GameManager& g, double bpm);

    void Update(GameManager& g);

    bool IsNewBeat() const { return _newBeat; }

    double BeatTimeToSecs(double beatTime) const {
        return beatTime * 60.0 / _bpm;
    }
    double SecsToBeatTime(double secs) const {
        return secs * _bpm / 60.0;
    }

    // These all return the values computed when Update() was called on the
    // clock. So they remain constant throughout the game frame.
    double GetBeatTimeFromEpoch() const;
    double GetDownBeatTime() const {
        return GetLastDownBeatTime(_currentBeatTime);
    }
    static double GetLastDownBeatTime(double beatTime) {
        return std::floor(beatTime);
    }
    static double GetNextDownBeatTime(double beatTime) {
        return std::ceil(beatTime);
    }
    static double GetBeatFraction(double beatTime) {
        double unused;
        return std::modf(beatTime, &unused);
    }
    // Denom == 1: next quarter note (beat)
    // Denom == 0.5: next 8th note. etc
    static double GetNextBeatDenomTime(double beatTime, double denom);
    
    double GetAudioTime() const {
        return _currentAudioTime;
    }
    double const GetBpm() const {
        return _bpm;
    }

    double _bpm = 120.0;

private:
    double _currentBeatTime = -1.0;
    double _currentAudioTime = -1.0;
    bool _newBeat = false;
};
