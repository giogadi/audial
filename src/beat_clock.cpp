#include "beat_clock.h"

#include <portaudio.h>

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