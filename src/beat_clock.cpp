#include "beat_clock.h"

#include "audio.h"

void BeatClock::Init(GameManager& g, double bpm, double sampleRate) {
    _bpm = bpm;
    _sampleRate = sampleRate;
    double audioTimeNow = Pa_GetStreamTime(g._audioContext->_stream);
    double beatTimeNow = audioTimeNow * (_bpm / 60.0);
    //_epochBeatTime = GetNextDownBeatTime(beatTimeNow);
    _epochBeatTime = beatTimeNow + 1;
}

void BeatClock::Update(GameManager& g) {
    double audioTime = Pa_GetStreamTime(g._audioContext->_stream);
    double beatTime = audioTime * (_bpm / 60.0);
    double downBeat = std::floor(beatTime);
    _newBeat = false;
    if (downBeat != std::floor(_currentBeatTime)) {
        _newBeat = true;
    }
    _currentBeatTime = beatTime;
    _currentAudioTime = audioTime;
}

double BeatClock::GetBeatTimeFromEpoch() const {
    return GetBeatTime() - _epochBeatTime;
}

double BeatClock::GetNextBeatDenomTime(double beatTime, double denom) {
    if (denom <= 0.0) {
        return beatTime;
    }
    return std::floor((beatTime / denom)) * denom + denom;
}
