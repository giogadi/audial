#include "step_sequencer.h"

#include <sstream>
#include <string_view>
#include <charconv>

#include "audio.h"
#include "game_manager.h"

void StepSequencerEntity::SetNextSeqStep(GameManager& g, int midiNote, float velocity) {
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
    _changeQueue.emplace(midiNote, velocity);
}

void StepSequencerEntity::Init(GameManager& g) {
    _midiSequence = _initialMidiSequence;
    _loopStartBeatTime = _initialLoopStartBeatTime;
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

    SeqStep seqStep = _midiSequence[_currentIx];
    if (!_changeQueue.empty()) {
        seqStep = _changeQueue.front();
        _midiSequence[_currentIx] = seqStep;
        _changeQueue.pop();
    }

    // Play the sound
    if (seqStep._midiNote >= 0) {
        audio::Event e;
        e.type = audio::EventType::NoteOn;
        e.channel = _channel;
        e.timeInTicks = 0;
        e.midiNote = seqStep._midiNote;
        e.velocity = seqStep._velocity;
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
    for (int ix = 0; ix < _initialMidiSequence.size(); ++ix) {
        if (ix > 0) {
            seqSs << " ";
        }
        SeqStep const& s = _initialMidiSequence[ix];
        seqSs << s._midiNote << ":" << s._velocity;
    }
    pt.PutString("sequence", seqSs.str().c_str());
    pt.PutDouble("step_length", _stepBeatLength);
    pt.PutInt("channel", _channel);
    pt.PutDouble("note_length", _noteLength);
    pt.PutDouble("start_time", _initialLoopStartBeatTime);
}

namespace {
int StoiOrDie(std::string_view str) {
    int x;
    auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.length(), x);
    assert(ec == std::errc());
    return x;
}
float StofOrDie(std::string_view str) {
    // WTF, doesn't work yet???
    // float x;
    // auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.length(), x);
    // assert(ec == std::errc());
    // return x;
    std::string ownedStr(str);
    return std::stof(ownedStr);
}
}

void StepSequencerEntity::LoadDerived(serial::Ptree pt) {
    std::string seq = pt.GetString("sequence");    
    std::stringstream seqSs(seq);
    while (!seqSs.eof()) {
        SeqStep step;
        std::string stepStr;
        seqSs >> stepStr;
        std::size_t delimIx = stepStr.find_first_of(':');
        if (delimIx == std::string::npos) {
            step._midiNote = std::stoi(stepStr);
            step._velocity = 1.f;
        } else {                        
            std::string_view midiNoteStr(stepStr.c_str(), delimIx);
            step._midiNote = StoiOrDie(midiNoteStr);
            std::string_view velStr = std::string_view(stepStr).substr(delimIx+1);
            step._velocity = StofOrDie(velStr);
        }
        _initialMidiSequence.push_back(step);
    }
    
    _stepBeatLength = pt.GetDouble("step_length");
    _channel = pt.GetInt("channel");
    _noteLength = pt.GetDouble("note_length");
    _initialLoopStartBeatTime = pt.GetDouble("start_time");
}
