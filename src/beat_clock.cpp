#include "beat_clock.h"

#include "audio_platform.h"

void BeatClock::Init(GameManager& g, double bpm) {
    _bpm = bpm;
    double audioTimeNow = g._audioContext->GetAudioTime();
    _currentAudioTime = audioTimeNow;
    _currentBeatTime = -1.0;
}

void BeatClock::Update(GameManager& g) {
    double audioTime = g._audioContext->GetAudioTime();
    double dtAudio = audioTime - _currentAudioTime;
    double dtBeat = dtAudio * (_bpm / 60.0);
    double beatTime = _currentBeatTime + dtBeat;
    double downBeat = std::floor(beatTime);
    _newBeat = false;
    if (downBeat != std::floor(_currentBeatTime)) {
        _newBeat = true;
    }
    _currentBeatTime = beatTime;
    _currentAudioTime = audioTime;
}

double BeatClock::GetBeatTimeFromEpoch() const {
    return _currentBeatTime;
}

double BeatClock::GetNextBeatDenomTime(double beatTime, double denom) {
    if (denom <= 0.0) {
        return beatTime;
    }
    return std::floor((beatTime / denom)) * denom + denom;
}
