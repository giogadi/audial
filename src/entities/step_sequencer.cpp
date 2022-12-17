#include "step_sequencer.h"

#include <sstream>
#include <string_view>
#include <charconv>

#include "audio.h"
#include "game_manager.h"
#include "midi_util.h"

void StepSequencerEntity::SetNextSeqStep(GameManager& g, SeqStep step) {
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
    _changeQueue.push(std::move(step));
}

void StepSequencerEntity::Init(GameManager& g) {
    _midiSequence = _initialMidiSequence = _initialMidiSequenceDoNotChange;
    _loopStartBeatTime = _initialLoopStartBeatTime;
}

void StepSequencerEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }
    
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
    audio::Event e;
    e.timeInTicks = 0;
    e.velocity = seqStep._velocity;
    e.type = audio::EventType::NoteOn;
    for (int i = 0; i < seqStep._midiNote.size(); ++i) {
        if (seqStep._midiNote[i] < 0) {
            break;
        }
        e.midiNote = seqStep._midiNote[i];
        for (int channel : _channels) {
            e.channel = channel;
            g._audioContext->AddEvent(e);
        }
    }
    double noteOffBeatTime = beatClock.GetBeatTime() + _noteLength;        
    e.type = audio::EventType::NoteOff;
    e.timeInTicks = beatClock.BeatTimeToTickTime(noteOffBeatTime);
    for (int i = 0; i < seqStep._midiNote.size(); ++i) {
        if (seqStep._midiNote[i] < 0) {
            break;
        }
        e.midiNote = seqStep._midiNote[i];
        for (int channel : _channels) {
            e.channel = channel;
            g._audioContext->AddEvent(e);
        }        
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
    std::string noteName;
    for (int ix = 0; ix < _initialMidiSequence.size(); ++ix) {
        if (ix > 0) {
            seqSs << " ";
        }
        SeqStep const& s = _initialMidiSequenceDoNotChange[ix];
        seqSs << "m";
        for (int noteIx = 0; noteIx < s._midiNote.size(); ++noteIx) {
            if (s._midiNote[noteIx] < 0 && noteIx > 0) {
                break;
            }
            if (noteIx > 0) {
                seqSs << ",";
            }
            GetNoteName(s._midiNote[noteIx], noteName);
            seqSs << noteName;
        }
        seqSs << ":" << s._velocity;
    }
    pt.PutString("sequence", seqSs.str().c_str());
    
    serial::Ptree channelsPt = pt.AddChild("channels");
    for (int c : _channels) {
        channelsPt.PutInt("channel", c);
    }
    pt.PutDouble("step_length", _stepBeatLength);
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
        // Chars [0,delimIx) are the notes delimited by ,
        int currentNoteIx = 0;
        int curIx = 0;
        bool continueLoop = true;
        while (continueLoop && curIx < stepStr.size()) {
            std::size_t noteDelimIx = stepStr.find_first_of(",:", curIx);
            if (noteDelimIx == curIx) {
                break;
            }
            std::string_view noteStr;
            if (noteDelimIx == std::string::npos) {
                noteStr = std::string_view(stepStr).substr(curIx);
                continueLoop = false;
            } else {
                noteStr = std::string_view(stepStr).substr(curIx, noteDelimIx - curIx);
                if (stepStr[noteDelimIx] == ':') {
                    continueLoop = false;
                } else {
                    curIx = noteDelimIx + 1;
                }
            }            
            int midiNote = -1;
            if (noteStr[0] == 'm') {
                midiNote = GetMidiNote(noteStr);
            } else {
                midiNote = StoiOrDie(noteStr);
            }
            if (currentNoteIx >= step._midiNote.size()) {
                printf("StepSequencer::Load: Too many notes!\n");
                break;
            }
            step._midiNote[currentNoteIx] = midiNote;
            ++currentNoteIx;
        }

        std::size_t velDelimIx = stepStr.find_first_of(':');
        if (velDelimIx != std::string::npos) {
            std::string_view velStr = std::string_view(stepStr).substr(velDelimIx+1);
            step._velocity = StofOrDie(velStr);
        }
       
        _initialMidiSequenceDoNotChange.push_back(step);
    }
    
    _stepBeatLength = pt.GetDouble("step_length");
    int channel = 0;
    if (pt.TryGetInt("channel", &channel)) {
        _channels.push_back(channel);
    } else {
        serial::Ptree channelsPt = pt.GetChild("channels");
        int numChildren = 0;
        serial::NameTreePair* children = channelsPt.GetChildren(&numChildren);
        _channels.reserve(numChildren);
        for (int i = 0; i < numChildren; ++i) {
            int channel = children[i]._pt.GetIntValue();
            _channels.push_back(channel);
        }
        delete[] children;
    }
    _noteLength = pt.GetDouble("note_length");
    _initialLoopStartBeatTime = pt.GetDouble("start_time");
}
