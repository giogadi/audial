#include "step_sequencer.h"

#include <sstream>
#include <string_view>
#include <charconv>

#include "imgui/imgui.h"

#include "audio.h"
#include "game_manager.h"
#include "midi_util.h"
#include "string_util.h"

void StepSequencerEntity::WriteSeqStep(SeqStep const& step, std::ostream& output) {
    std::string noteName;
    for (int i = 0, n = step._midiNote.size(); i < n; ++i) {
        int midiNote = step._midiNote[i];
        if (midiNote < 0 && i > 0) {
            break;
        }
        if (i > 0) {
            output << ",";
        }
        if (midiNote < 0) {
            output << "-1";
        } else {
            GetNoteName(midiNote, noteName);
            output << "m" << noteName;
        }
    }
    output << ":" << step._velocity;
}

bool StepSequencerEntity::TryReadSeqStep(std::istream& input, SeqStep& step) {
    std::string stepStr;
    input >> stepStr;
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
            bool success = MaybeGetMidiNoteFromStr(noteStr.substr(1), midiNote);
            if (!success) {
                return false;
            }
        } else {
            bool success = string_util::MaybeStoi(noteStr, midiNote);
            if (!success) {
                return false;
            }
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
        bool success = string_util::MaybeStof(velStr, step._velocity);
        if (!success) {
            return false;
        }
    }

    // Returns false if we never read a note
    return currentNoteIx > 0;
}

void StepSequencerEntity::LoadSequenceFromInput(std::istream& input, std::vector<SeqStep>& sequence) {
    sequence.clear();
    while (!input.eof()) {
        SeqStep step;
        bool hasStep = TryReadSeqStep(input, step);
        if (hasStep) {
            sequence.push_back(step);
        }
    }
}



void StepSequencerEntity::SetNextSeqStep(GameManager& g, SeqStep step, StepSaveType saveType) {
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
    SeqStepChange change;
    change._step = step;
    switch (saveType) {
        case StepSaveType::Temporary:
            change._temporary = true;
            break;
        case StepSaveType::Permanent:
            change._temporary = false;
            break;
    }
    change._changeNote = true;
    change._changeVelocity = true;
    _changeQueue.push(std::move(change));
}

void StepSequencerEntity::SetNextSeqStepVelocity(GameManager& g, float v, StepSaveType saveType) {
    SeqStepChange change;
    change._step._velocity = v;
    switch (saveType) {
        case StepSaveType::Temporary:
            change._temporary = true;
            break;
        case StepSaveType::Permanent:
            change._temporary = false;
            break;
    }
    change._changeNote = false;
    change._changeVelocity = true;
    _changeQueue.push(std::move(change));
}

void StepSequencerEntity::SetAllVelocitiesPermanent(float newValue) {
    assert(_permanentSequence.size() == _tempSequence.size());
    for (int i = 0, n = _permanentSequence.size(); i < n; ++i) {
        _tempSequence[i]._velocity = _permanentSequence[i]._velocity = newValue;
    }
}

void StepSequencerEntity::SetAllStepsPermanent(SeqStep const& newStep) {
    assert(_permanentSequence.size() == _tempSequence.size());
    for (int i = 0, n = _permanentSequence.size(); i < n; ++i) {
        _tempSequence[i] = _permanentSequence[i] = newStep;
    }
}

void StepSequencerEntity::SetSequencePermanent(std::vector<SeqStep> newSequence) {
    assert(_permanentSequence.size() == _tempSequence.size());
    assert(_permanentSequence.size() == newSequence.size());
    for (int i = 0, n = newSequence.size(); i < n; ++i) {
        _tempSequence[i] = _permanentSequence[i] = newSequence[i];
    }
}

void StepSequencerEntity::InitDerived(GameManager& g) {
    _mute = _startMute;
    _tempSequence = _permanentSequence = _initialMidiSequenceDoNotChange;
    _loopStartBeatTime = _initialLoopStartBeatTime;
    _maxNumVoices = _initMaxNumVoices;
}

void StepSequencerEntity::Update(GameManager& g, float dt) {    
    if (_permanentSequence.empty()) {
        return;
    }
    
    BeatClock const& beatClock = *g._beatClock;

    double const beatTime = beatClock.GetBeatTimeFromEpoch();

    // Play sound on the first frame at-or-after the target beat time
    double nextStepBeatTime = _loopStartBeatTime + _currentIx * _stepBeatLength;

    if (beatTime < nextStepBeatTime) {
        return;
    }

    if (!_changeQueue.empty()) {
        SeqStepChange const& change = _changeQueue.front();
        SeqStep& tempStep = _tempSequence[_currentIx];        
        if (change._changeNote) {
            tempStep._midiNote = change._step._midiNote;
        }
        if (change._changeVelocity) {
            tempStep._velocity = change._step._velocity;
        }
        if (!change._temporary) {
            _permanentSequence[_currentIx] = tempStep;
        }
        _changeQueue.pop();
    }   

    // Play the sound
    if (!_mute && (!g._editMode || !_editorMute)) {
        SeqStep const& seqStep = _tempSequence[_currentIx];
        int numVoices = seqStep._midiNote.size();
        if (_maxNumVoices >= 0) {
            numVoices = std::min(numVoices, _maxNumVoices);
        }
        for (int i = 0; i < numVoices; ++i) {
            if (seqStep._midiNote[i] < 0 || seqStep._velocity <= 0.f) {
                numVoices = i;
                break;
            }
        }
        if (_isSynth) {
            audio::Event e;
            e.timeInTicks = 0;
            e.velocity = seqStep._velocity;
            e.type = audio::EventType::NoteOn;
            for (int i = 0; i < numVoices; ++i) {
                e.midiNote = seqStep._midiNote[i];
                for (int channel : _channels) {
                    e.channel = channel;
                    g._audioContext->AddEvent(e);
                }
            }
            double noteOffBeatTime = beatClock.GetBeatTime() + _noteLength;        
            e.type = audio::EventType::NoteOff;
            e.timeInTicks = beatClock.BeatTimeToTickTime(noteOffBeatTime);
            for (int i = 0; i < numVoices; ++i) {
                e.midiNote = seqStep._midiNote[i];
                for (int channel : _channels) {
                    e.channel = channel;
                    g._audioContext->AddEvent(e);
                }        
            }
        } else {
            audio::Event e;
            e.timeInTicks = 0;
            e.pcmVelocity = seqStep._velocity;
            e.type = audio::EventType::PlayPcm;
            e.loop = false;
            for (int i = 0; i < numVoices; ++i) {
                e.pcmSoundIx = seqStep._midiNote[i];
                g._audioContext->AddEvent(e);
            }
        }
    }
    
    // After the playing the sound, reset that seq element to the initial
    // value
    _tempSequence[_currentIx] = _permanentSequence[_currentIx];

    ++_currentIx;

    if (_currentIx >= _permanentSequence.size()) {
        _currentIx = 0;
        _loopStartBeatTime += _permanentSequence.size() * _stepBeatLength;
    }
}

void StepSequencerEntity::SaveDerived(serial::Ptree pt) const {
    std::stringstream seqSs;
    std::string noteName;
    for (int ix = 0; ix < _initialMidiSequenceDoNotChange.size(); ++ix) {
        if (ix > 0) {
            seqSs << " ";
        }
        SeqStep const& s = _initialMidiSequenceDoNotChange[ix];
        for (int noteIx = 0; noteIx < s._midiNote.size(); ++noteIx) {
            if (s._midiNote[noteIx] < 0) {
                if (noteIx == 0) {
                    seqSs << "-1";
                }
                break;
            }
            if (noteIx > 0) {
                seqSs << ",";
            }
            GetNoteName(s._midiNote[noteIx], noteName);
            seqSs << "m" << noteName;
        }
        seqSs << ":" << s._velocity;
    }
    pt.PutString("sequence", seqSs.str().c_str());

    pt.PutBool("start_mute", _startMute);
    
    serial::Ptree channelsPt = pt.AddChild("channels");
    for (int c : _channels) {
        channelsPt.PutInt("channel", c);
    }
    pt.PutDouble("step_length", _stepBeatLength);
    pt.PutDouble("note_length", _noteLength);
    pt.PutDouble("start_time", _initialLoopStartBeatTime);
    pt.PutBool("is_synth", _isSynth);
    pt.PutBool("editor_mute", _editorMute);
    pt.PutInt("max_voices", _initMaxNumVoices);
}

void StepSequencerEntity::LoadDerived(serial::Ptree pt) {
    std::string seq = pt.GetString("sequence");    
    std::stringstream seqSs(seq);
    LoadSequenceFromInput(seqSs, _initialMidiSequenceDoNotChange);

    pt.TryGetBool("start_mute", &_startMute);
    
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
    _isSynth = true;
    pt.TryGetBool("is_synth", &_isSynth);
    pt.TryGetBool("editor_mute", &_editorMute);
    pt.TryGetInt("max_voices", &_initMaxNumVoices);
}

ne::BaseEntity::ImGuiResult StepSequencerEntity::ImGuiDerived(GameManager& g) {
    ImGui::Checkbox("Mute in Editor", &_editorMute);
    ImGui::Checkbox("Start mute", &_startMute);
    return ImGuiResult::Done;
}
