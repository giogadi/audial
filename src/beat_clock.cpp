#include "beat_clock.h"

#include "audio.h"

void BeatClock::Init(GameManager& g, double bpm, double sampleRate) {
    _bpm = bpm;
    _sampleRate = sampleRate;
    double audioTimeNow = g._audioContext->_state.GetTimeInSeconds();
    double beatTimeNow = audioTimeNow * (_bpm / 60.0);
    _epochBeatTime = GetNextDownBeatTime(beatTimeNow);
}

void BeatClock::Update(GameManager& g) {
    double audioTime = g._audioContext->_state.GetTimeInSeconds();
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

double BeatClock::GetBeatTimeFromEpoch() const {
    return GetBeatTime() - _epochBeatTime;
}
