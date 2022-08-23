#pragma once

#include <cmath>

// typedef PaStream;

class BeatClock {
public:
    BeatClock(double bpm, double sampleRate, void* stream)
        : _bpm(bpm)
        , _paStream(stream)
        , _sampleRate(sampleRate) {}

    void Update();

    bool IsNewBeat() const { return _newBeat; }

    unsigned long BeatTimeToTickTime(double beatTime) const {
        return beatTime * (60.0 / _bpm) * _sampleRate;
    }

    double TickTimeToBeatTime(unsigned long tickTime) const {
        return (double)(tickTime * _bpm) / (60.0 * _sampleRate);
    }

    // These all return the values computed when Update() was called on the
    // clock. So they remain constant throughout the game frame.
    double GetBeatTime() const {
        return _currentBeatTime;
    }
    double GetDownBeatTime() const {
        return GetLastDownBeatTime(_currentBeatTime);
    }
    static double GetLastDownBeatTime(double beatTime) {
        return std::floor(beatTime);
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

private:
    double _bpm = 120.0;
    double _currentBeatTime = -1.0;
    double _currentAudioTime = -1.0;
    unsigned long _currentTickTime = 0;
    bool _newBeat = false;
    void* _paStream = nullptr;
    double _sampleRate = 44100.0;
};