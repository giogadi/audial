#include "step_sequencer.h"

#include <sstream>

#include "audio.h"
#include "game_manager.h"

void StepSequencerEntity::SetNextSeqStep(GameManager& g, int midiNote) {
    // BeatClock const& beatClock = *g._beatClock;
    // double const beatTime = beatClock.GetBeatTimeFromEpoch();
    // double nextStepBeatTime = _loopStartBeatTime + _currentIx * _stepBeatLength;
    // if (nextStepBeatTime - beatTime > 0.75 * _stepBeatLength) {
    //     // If this happens, just cancel the other note being played.
    //     if (_currentIx > 0) {
    //         // Shut off other sound now!!
    //         --_currentIx;            
    //     }
    // }
    // _midiSequence[_currentIx] = midiNote;
    _changeQueue.push(midiNote);
}

void StepSequencerEntity::Init(GameManager& g) {
    _midiSequence = _initialMidiSequence;    
}

void StepSequencerEntity::Update(GameManager& g, float dt) {
    if (_midiSequence.empty()) {
        return;
    }
    
    BeatClock const& beatClock = *g._beatClock;

    double const beatTime = beatClock.GetBeatTimeFromEpoch();

    // Play sound on the first frame at-or-after the target beat time
    double nextStepBeatTime = _loopStartBeatTime + _currentIx * _stepBeatLength;

    if (beatTime < nextStepBeatTime) {
        return;
    }

    int midiNote = _midiSequence[_currentIx];
    if (!_changeQueue.empty()) {
        midiNote = _changeQueue.front();
        _midiSequence[_currentIx] = midiNote;
        _changeQueue.pop();
    }

    // Play the sound
    if (_midiSequence[_currentIx] >= 0) {
        audio::Event e;
        e.type = audio::EventType::NoteOn;
        e.channel = _channel;
        e.timeInTicks = 0;
        e.midiNote = midiNote;
        g._audioContext->AddEvent(e);

        double noteOffBeatTime = beatClock.GetBeatTime() + _noteLength;        
        e.type = audio::EventType::NoteOff;
        e.timeInTicks = beatClock.BeatTimeToTickTime(noteOffBeatTime);
        g._audioContext->AddEvent(e);
    }

    // After the playing the sound, maybe reset that seq element to the initial
    // value
    if (_resetToInitialAfterPlay) {
        _midiSequence[_currentIx] = _initialMidiSequence[_currentIx];
    }

    ++_currentIx;

    if (_currentIx >= _midiSequence.size()) {
        _currentIx = 0;
        _loopStartBeatTime += _midiSequence.size() * _stepBeatLength;
    }
}

void StepSequencerEntity::SaveDerived(serial::Ptree pt) const {
    std::stringstream seqSs;
    for (int m : _initialMidiSequence) {
        seqSs << m << " ";
    }
    pt.PutString("sequence", seqSs.str().c_str());
    pt.PutDouble("step_length", _stepBeatLength);
    pt.PutInt("channel", _channel);
    pt.PutDouble("note_length", _noteLength);
    pt.PutDouble("start_time", _loopStartBeatTime);
}

void StepSequencerEntity::LoadDerived(serial::Ptree pt) {
    std::string seq = pt.GetString("sequence");    
    std::stringstream seqSs(seq);
    while (!seqSs.eof()) {
        int m;
        seqSs >> m;
        _initialMidiSequence.push_back(m);
    }
    
    _stepBeatLength = pt.GetDouble("step_length");
    _channel = pt.GetInt("channel");
    _noteLength = pt.GetDouble("note_length");
    _loopStartBeatTime = pt.GetDouble("start_time");
}
