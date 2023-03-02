#pragma once

#include <cmath>
#include "game_manager.h"

class BeatClock {
public:
    void Init(GameManager& g, double bpm, double sampleRate);

    void Update(GameManager& g);

    bool IsNewBeat() const { return _newBeat; }

    unsigned long BeatTimeToTickTime(double beatTime) const {
        return static_cast<unsigned long>(beatTime * (60.0 / _bpm) * _sampleRate);
    }

    double TickTimeToBeatTime(unsigned long tickTime) const {
        return (double)(tickTime * _bpm) / (60.0 * _sampleRate);
    }

    unsigned long EpochBeatTimeToTickTime(double beatTime) const {
        return BeatTimeToTickTime(beatTime + _epochBeatTime);
    }

    // These all return the values computed when Update() was called on the
    // clock. So they remain constant throughout the game frame.
    double GetBeatTime() const {
        return _currentBeatTime;
    }
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
    static double GetNextBeatDenomTime(double beatTime, double denom) {
        return std::floor((beatTime / denom)) * denom + denom;
    }
    unsigned long GetTimeInTicks() const {
        return _currentTickTime;
    }
    double GetAudioTime() const {
        return _currentAudioTime;
    }
    double const GetBpm() const {
        return _bpm;
    }

private:
    double _epochBeatTime = -1.0;
    double _bpm = 120.0;
    double _currentBeatTime = -1.0;
    double _currentAudioTime = -1.0;
    unsigned long _currentTickTime = 0;
    bool _newBeat = false;
    double _sampleRate = 44100.0;
};
