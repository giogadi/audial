#include "beat_clock.h"

#include <portaudio.h>

void BeatClock::Init(double bpm, double sampleRate, void* stream) {
    _bpm = bpm;
    _paStream = stream;
    _sampleRate = sampleRate;
    double audioTimeNow = Pa_GetStreamTime(_paStream);
    double beatTimeNow = audioTimeNow * (_bpm / 60.0);
    _epochBeatTime = GetNextDownBeatTime(beatTimeNow);
}

void BeatClock::Update() {
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

double BeatClock::GetBeatTimeFromEpoch() const {
    return GetBeatTime() - _epochBeatTime;
}