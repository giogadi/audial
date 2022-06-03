#pragma once

#include <cmath>

#include <portaudio.h>

class BeatClock {
public:
    BeatClock(double bpm, double sampleRate, PaStream* stream)
        : _bpm(bpm)
        , _sampleRate(sampleRate)
        , _paStream(stream) {}

    void Update() {
        double audioTime = Pa_GetStreamTime(_paStream);
        double beatTime = audioTime * (_bpm / 60.0);
        double downBeat = std::floor(beatTime);
        _newBeat = false;
        if (downBeat != std::floor(_currentBeatTime)) {
            _newBeat = true;
        }
        _currentBeatTime = beatTime;
        _currentAudioTime = audioTime;
        _currentTickTime = (unsigned long) std::round(audioTime * _sampleRate);
    }

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
        return std::floor(_currentBeatTime);
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
    PaStream* _paStream = nullptr;
    double _sampleRate = 44100.0;
};