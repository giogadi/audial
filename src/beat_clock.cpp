#include "beat_clock.h"

#include "audio.h"

void BeatClock::Init(GameManager& g, double bpm, double sampleRate) {
    _bpm = bpm;
    _sampleRate = sampleRate;
    double audioTimeNow = Pa_GetStreamTime(g._audioContext->_stream);
    _currentAudioTime = audioTimeNow;
    _currentBeatTime = -1.0;
}

void BeatClock::Update(GameManager& g) {
    double audioTime = Pa_GetStreamTime(g._audioContext->_stream);
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
