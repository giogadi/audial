#include "step_sequencer.h"

#include <sstream>
#include <string_view>
#include <charconv>

#include "imgui/imgui.h"

#include "audio_platform.h"
#include "game_manager.h"
#include "midi_util.h"
#include "string_util.h"
#include "math_util.h"
#include "sound_bank.h"
#include "imgui_vector_util.h"
#include "serial_vector_util.h"

extern GameManager gGameManager;

void StepSequencerEntity::SynthParamValue::Save(serial::Ptree pt) const {
    pt.PutBool("active", _active);
    pt.PutFloat("v", _value);
}

void StepSequencerEntity::SynthParamValue::Load(serial::Ptree pt) {
    pt.TryGetBool("active", &_active);
    _value = pt.GetFloat("v");
}

bool StepSequencerEntity::SynthParamValue::ImGui() {
    bool changed = false;
    changed = ImGui::Checkbox("Active", &_active) || changed;
    if (_active) {
        changed = ImGui::InputFloat("Value", &_value) || changed;
    }
    return changed;
}

void StepSequencerEntity::MidiNoteAndVelocity::Save(serial::Ptree pt) const {
    pt.PutInt("note", _note);
    pt.PutFloat("v", _v);
}

void StepSequencerEntity::MidiNoteAndVelocity::Load(serial::Ptree pt) {
    pt.TryGetInt("note", &_note);
    pt.TryGetFloat("v", &_v);
}

void StepSequencerEntity::SeqStep::Save(serial::Ptree pt) const {
    {
        serial::Ptree notesPt = pt.AddChild("notes");
        for (int ii = 0; ii < SeqStep::kNumNotes; ++ii) {
            serial::SaveInNewChildOf(notesPt, "n", _notes[ii]);
        }
    }
    {
        serial::Ptree paramsPt = pt.AddChild("params");
        for (int ii = 0; ii < kNumParamTracks; ++ii) {
            if (!_params[ii]._active) {
                continue;
            }
            serial::SaveInNewChildOf(paramsPt, "p", _params[ii]);
        }
    }
}

void StepSequencerEntity::SeqStep::Load(serial::Ptree pt) {
    int constexpr kFirstMultiVelocityVersion = 13;
    float oldVelocity = 1.f;
    if (pt.GetVersion() < kFirstMultiVelocityVersion) {
        oldVelocity = pt.GetFloat("v"); 
    }

    {
        serial::Ptree notesPt = pt.GetChild("notes");
        int numNotes = 0;
        serial::NameTreePair* children = notesPt.GetChildren(&numNotes);
        if (numNotes > SeqStep::kNumNotes) {
            printf("StepSequencerEntity::SeqStep::Load: HEY PROBLEM: TOO MANY NOTES (%d)\n", numNotes);
            numNotes = SeqStep::kNumNotes;
        }
        if (pt.GetVersion() < kFirstMultiVelocityVersion) {
            for (int ii = 0; ii < numNotes; ++ii) {
                serial::NameTreePair* child = &children[ii];
                _notes[ii]._note = child->_pt.GetIntValue();
                _notes[ii]._v = oldVelocity; 
            }
        } else {
            for (int ii = 0; ii < numNotes; ++ii) {
                serial::NameTreePair* child = &children[ii];
                _notes[ii].Load(child->_pt); 
            }
        }
        delete[] children;
    }

    {
        for (int ii = 0; ii < kNumParamTracks; ++ii) {
            _params[ii] = SynthParamValue();
        }
        serial::Ptree paramsPt = pt.GetChild("params");
        int numParams = 0;
        serial::NameTreePair* children = paramsPt.GetChildren(&numParams);
        if (numParams > kNumParamTracks) {
            printf("StepSequencerEntity::SeqStep::Load: HEY PROBLEM: TOO MANY PARAMS (%d)\n", numParams);
            numParams = kNumParamTracks;
        }
        for (int ii = 0; ii < numParams; ++ii) {
            serial::NameTreePair* child = &children[ii];
            _params[ii].Load(child->_pt);
        }
        delete[] children;
    }
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
        if (currentNoteIx >= step._notes.size()) {
            printf("StepSequencer::Load: Too many notes!\n");
            break;
        }
        step._notes[currentNoteIx]._note = midiNote;
        ++currentNoteIx;
    }
    std::size_t velDelimIx = stepStr.find_first_of(':');
    if (velDelimIx != std::string::npos) {
        std::string_view velStr = std::string_view(stepStr).substr(velDelimIx+1);
        float oldVelocity;
        bool success = string_util::MaybeStof(velStr, oldVelocity);
        if (success) {
            for (int ii = 0; ii < SeqStep::kNumNotes; ++ii) {
                step._notes[ii]._v = oldVelocity;
            }
        } else {
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

bool StepSequencerEntity::SeqImGui(char const* label, bool drumKit, std::vector<SeqStep>& sequence) {
    int constexpr kMaxRows = 256;    

    std::vector<char const*> const& soundNames = gGameManager._soundBank->_soundNames;
    int const numSounds = static_cast<int>(soundNames.size());

    bool changed = false;

    static std::vector<SeqStep> sClipboardSequence;
    if (ImGui::Button("Copy")) {
        sClipboardSequence = sequence;
    }
    ImGui::SameLine();
    if (ImGui::Button("Paste")) {
        sequence = sClipboardSequence;
        changed = true;
    }
    
    int numSteps = sequence.size();
    if (ImGui::InputInt("# steps", &numSteps, 1, 1, ImGuiInputTextFlags_EnterReturnsTrue)) {
        numSteps = math_util::Clamp(numSteps, 0, kMaxRows);
        sequence.resize(numSteps);
        changed = true;
    }

    static float allVelocity = 1.f;
    static int allVelocityNoteIx = 0;
    float const kTextBaseWidth = ImGui::CalcTextSize("A").x;
    ImGui::PushItemWidth(kTextBaseWidth * 8);
    ImGui::InputFloat("V##allV", &allVelocity);
    ImGui::SameLine();
    ImGui::InputInt("Note ix", &allVelocityNoteIx);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Set all velocities")) {
        if (allVelocityNoteIx >= 0 && allVelocityNoteIx < SeqStep::kNumNotes) {
            for (int i = 0, n = sequence.size(); i < n; ++i) {
                sequence[i]._notes[allVelocityNoteIx]._v = allVelocity;
            }
        }
        changed = true;
    }

    float const textBaseHeight = ImGui::GetTextLineHeightWithSpacing();
    static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollX;
    ImVec2 outerSize = ImVec2(0.f, textBaseHeight * 8);
    int constexpr kStepColIx = 0;
    int constexpr kFirstNoteColIx = 1;
    int constexpr kNumNotes = StepSequencerEntity::SeqStep::kNumNotes;
    int constexpr kParam0ColIx = kFirstNoteColIx + kNumNotes;
    int constexpr kNumColumns = kParam0ColIx + kNumParamTracks;
    if (ImGui::BeginTable(label, kNumColumns, flags, outerSize)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Step", ImGuiTableColumnFlags_None);
        char noteStr[] = "NoteX";
        assert(kNumNotes < 10);
        for (int i = 0; i < kNumNotes; ++i) {
            sprintf(noteStr, "Note%d", i);
            ImGui::TableSetupColumn(noteStr, ImGuiTableColumnFlags_None);
        }
        char paramStr[] = "ParamX";
        for (int ii = 0; ii < kNumParamTracks; ++ii) {
            sprintf(paramStr, "Param%d", ii);
            ImGui::TableSetupColumn(paramStr, ImGuiTableColumnFlags_None);
        }
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(sequence.size());        

        int constexpr kStringSize = 4;
        static char sNoteStrings[kMaxRows][kNumNotes][kStringSize] = {};        
        
        while (clipper.Step()) {
            // assert(clipper.DisplayStart < sequence.size());
            assert(clipper.DisplayEnd <= sequence.size());
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                ImGui::TableNextRow();
                ImGui::PushID(row);
                ImGui::TableSetColumnIndex(kStepColIx);
                ImGui::Text("%d", row);
                for (int noteIx = 0; noteIx < kNumNotes; ++noteIx) {
                    int columnIx = kFirstNoteColIx + noteIx;
                    ImGui::TableSetColumnIndex(columnIx);
                    ImGui::PushID(columnIx);
                    int midiNote = sequence[row]._notes[noteIx]._note;
                    if (drumKit) {
                        char const* preview = nullptr;
                        char const* kNoneStr = "None";
                        if (midiNote < 0 || midiNote >= numSounds) {
                            preview = kNoneStr;
                        } else {
                            preview = soundNames[midiNote];
                        }
                        if (ImGui::BeginCombo("", preview)) {
                            bool selected = ImGui::Selectable(kNoneStr, midiNote < 0);
                            if (selected) {                                
                                midiNote = -1;
                                sequence[row]._notes[noteIx]._note = midiNote;
                                changed = true;
                            }
                            for (int soundIx = 0; soundIx < numSounds; ++soundIx) {
                                char const* soundName = soundNames[soundIx];
                                bool selected = ImGui::Selectable(soundName, midiNote == soundIx);
                                if (selected) {
                                    midiNote = soundIx;
                                    sequence[row]._notes[noteIx]._note = midiNote;
                                    changed = true;
                                }
                            }
                            ImGui::EndCombo();
                        }                        
                    } else {
                        // not drumkit
                        char* noteStr = sNoteStrings[row][noteIx];
                        GetNoteName(midiNote, noteStr);
                        if (ImGui::InputText("", noteStr, kStringSize+1, ImGuiInputTextFlags_EnterReturnsTrue)) {
                            int len = strlen(noteStr);
                            std::string_view noteStrView(noteStr, len);
                            midiNote = GetMidiNote(noteStrView);
                            sequence[row]._notes[noteIx]._note = midiNote;
                            changed = true;
                            // GetNoteName(midiNote, noteStr);
                        }
                    }
                    ImGui::SameLine();
                    ImGui::InputFloat("##vel", &sequence[row]._notes[noteIx]._v);
                    ImGui::PopID();
                }
                                
                for (int paramIx = 0; paramIx < kNumParamTracks; ++paramIx) {
                    int const columnIx = kParam0ColIx + paramIx;
                    ImGui::TableSetColumnIndex(columnIx);
                    ImGui::PushID(columnIx);
                    SynthParamValue* param = &sequence[row]._params[paramIx];
                    if (ImGui::Checkbox("##active", &param->_active)) {
                        changed = true;
                    }
                    if (param->_active) {                        
                        //ImGui::PushItemWidth(kTextBaseWidth * 8);
                        //ImGui::PopItemWidth();
                        ImGui::SameLine();
                        if (ImGui::InputFloat("##v", &param->_value)) {
                            changed = true;
                        }
                    }

                    ImGui::PopID();
                }

                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }

    return changed;
}

void StepSequencerEntity::EnqueueChange(SeqStepChange const& change) {
    _changeQueue[_changeQueueTailIx] = change;
    _changeQueueTailIx = (_changeQueueTailIx + 1) % kChangeQueueSize;
    if (_changeQueueCount == kChangeQueueSize) {
        _changeQueueHeadIx = (_changeQueueHeadIx + 1) % kChangeQueueSize;
    } else {
        ++_changeQueueCount;
    }
}


void StepSequencerEntity::SetNextSeqStep(GameManager& g, SeqStep step, StepSaveType saveType, bool changeNote, bool changeVelocity) {
    if (_enableLateChanges && _changeQueueCount == 0 && saveType == StepSaveType::Temporary && _quantizeTempStepChanges) {
        double nextStepBeatTime = _loopStartBeatTime + _currentIx * _stepBeatLength;
        double beatTime = gGameManager._beatClock->GetBeatTimeFromEpoch();
        double timeSinceLastNote = beatTime - _lastPlayedNoteTime;
        bool longEnoughSinceLast = timeSinceLastNote > 0.75*_stepBeatLength;
        double stepLengthSlackFactor = 0.25;
        bool check1 = (beatTime < nextStepBeatTime) && (nextStepBeatTime - beatTime > (1-stepLengthSlackFactor)*_stepBeatLength);
        bool check2 = (beatTime > nextStepBeatTime) && beatTime - nextStepBeatTime < stepLengthSlackFactor*_stepBeatLength;
        if (longEnoughSinceLast && (check1 || check2)) {
#if 0
            printf("HOWDY! ");
            printf("%f %f %f %f\n", beatTime, nextStepBeatTime, nextStepBeatTime - beatTime, _lastPlayedNoteTime);
#endif
            SeqStep toPlay = _tempSequence[_currentIx];
            if (changeNote) {
                for (int ii = 0; ii < toPlay._notes.size(); ++ii) {
                    toPlay._notes[ii]._note = step._notes[ii]._note;
                }
            }
            if (changeVelocity) {
                for (int ii = 0; ii < toPlay._notes.size(); ++ii) {
                    toPlay._notes[ii]._v = step._notes[ii]._v;
                }
            }
            toPlay._params = step._params;
            PlayStep(gGameManager, toPlay);
            // TODO!!!! THIS FLOW DOESN'T HANDLE PERMANENT CHANGES!!!!
            return;
        } else {
#if 0
            double frac = gGameManager._beatClock->GetBeatFraction(nextStepBeatTime);
            if (std::abs(frac - 0.25) < 0.00001 || std::abs(frac - 0.75) < 0.00001) {
                printf("***** ");
            } 
            printf("%f %f %f %f\n", beatTime, nextStepBeatTime, nextStepBeatTime - beatTime, _lastPlayedNoteTime);
#endif
        }
    }

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
    change._changeNote = changeNote;
    change._changeVelocity = changeVelocity;
    if (change._temporary && !_quantizeTempStepChanges) {
        SeqStep toPlay = _tempSequence[_currentIx];
        if (changeNote) {
            for (int ii = 0; ii < toPlay._notes.size(); ++ii) {
                toPlay._notes[ii]._note = step._notes[ii]._note;
            }
        }
        if (changeVelocity) {
            for (int ii = 0; ii < toPlay._notes.size(); ++ii) {
                toPlay._notes[ii]._v = step._notes[ii]._v;
            }
        }
        toPlay._params = step._params;
        PlayStep(gGameManager, toPlay);
    } else {
        EnqueueChange(change);
    }    
}

void StepSequencerEntity::SetNextSeqStepVelocity(GameManager& g, float v, StepSaveType saveType) {
    SeqStepChange change;
    for (int ii = 0; ii < change._step._notes.size(); ++ii) {
        change._step._notes[ii]._v = v;
    }
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
    EnqueueChange(change);
}

void StepSequencerEntity::SetAllVelocitiesPermanent(float newValue, int trackIx) {
    assert(_permanentSequence.size() == _tempSequence.size());
    for (int i = 0, n = _permanentSequence.size(); i < n; ++i) {
        if (trackIx < 0 || trackIx >= SeqStep::kNumNotes) {
            for (int jj = 0; jj < _permanentSequence[i]._notes.size(); ++jj) {
                _tempSequence[i]._notes[jj]._v = _permanentSequence[i]._notes[jj]._v = newValue;
            }
        } else {
            _tempSequence[i]._notes[trackIx]._v = _permanentSequence[i]._notes[trackIx]._v = newValue;
        }
    }
}

void StepSequencerEntity::SetAllStepsPermanent(SeqStep const& newStep) {
    assert(_permanentSequence.size() == _tempSequence.size());
    for (int i = 0, n = _permanentSequence.size(); i < n; ++i) {
        _tempSequence[i] = _permanentSequence[i] = newStep;
    }
}

void StepSequencerEntity::SetSequencePermanent(std::vector<SeqStep> const& newSequence) {
    bool sizeChanged = newSequence.size() != _permanentSequence.size();
    if (sizeChanged) {
        _seqNeedsReset = true;
    }
    _tempSequence = _permanentSequence = newSequence;    
}

void StepSequencerEntity::SetSequencePermanentWithStartOffset(std::vector<SeqStep> const& newSequence) {
    assert(_permanentSequence.size() == _tempSequence.size());
    assert(_permanentSequence.size() == newSequence.size());

    // HUGE HACK I'M SO SORRY. ASSUMES WE'RE QUANTIZING TO 1 BEAT AND THAT STEPS ARE 16th NOTES.
    // maps to previous beat.
    if (std::abs(_stepBeatLength - 0.25) > 0.00001) {
        printf("StepSequencerEntity::SetSequencePermanentWithStartOffset: WARNING I only support 0.25 beat lengths, but this beat length is %f\n", _stepBeatLength);
    } else {
        int startIx = ((4 * (_currentIx / 4)) + 4) % newSequence.size();
        for (int i = 0, n = newSequence.size(); i < n; ++i) {
            int ix = (startIx + i) % n;
            _tempSequence[ix] = _permanentSequence[ix] = newSequence[i];
        }
    }
}

void StepSequencerEntity::InitDerived(GameManager& g) {
    _mute = _startMute;
    _tempSequence = _permanentSequence = _initialMidiSequenceDoNotChange;
    _maxNumVoices = _initMaxNumVoices;
    _gain = _initGain;
    _seqNeedsReset = true;
}

void StepSequencerEntity::PlayStep(GameManager& g, SeqStep const& seqStep) {
    if (_mute || (g._editMode && _editorMute)) {
        return;
    }

    for (int ii = 0; ii < kNumParamTracks; ++ii) {
        if (!_paramTrackTypes[ii]._active || _paramTrackTypes[ii]._type == audio::SynthParamType::Count) {
            continue;
        }
        SynthParamValue const* param = &seqStep._params[ii];
        if (!param->_active) {
            continue;
        }
        audio::Event e;
        e.delaySecs = 0.0;
        e.type = audio::EventType::SynthParam;
        e.param = _paramTrackTypes[ii]._type;
        e.paramChangeTimeSecs = 0.0;
        e.newParamValue = param->_value;
        for (int channel : _channels) {
            e.channel = channel;
            g._audioContext->AddEvent(e);
        }
    }

    std::array<MidiNoteAndVelocity, SeqStep::kNumNotes> midiNotes;  // packed in
    int numPlayedNotes = 0;
    for (int ii = 0; ii < seqStep._notes.size(); ++ii) {
        if (_maxNumVoices > -1 && numPlayedNotes >= _maxNumVoices) {
            break;
        }
        if (seqStep._notes[ii]._note >= 0) {
            // TODO: do we need to check for velocity == 0 here too?
            midiNotes[numPlayedNotes++] = seqStep._notes[ii];
        }
    }

    if (midiNotes[0]._note > -1) {
        // TODO: this isn't _quite_ it, but almost
        _lastPlayedNoteTime = g._beatClock->GetBeatTimeFromEpoch();
    }

    if (_isSynth) {
        audio::Event e;
        e.delaySecs = 0.0;
        e.type = audio::EventType::NoteOn; 
        static int sNoteOnId = 1;
        e.noteOnId = sNoteOnId++;
        for (int i = 0; i < numPlayedNotes; ++i) {
            e.midiNote = midiNotes[i]._note;
            e.velocity = midiNotes[i]._v * _gain;
            if (_primePorta) {
                e.primePortaMidiNote = e.midiNote;
            }
            for (int channel : _channels) {
                e.channel = channel;
                g._audioContext->AddEvent(e);
            }
        }
        e.type = audio::EventType::NoteOff;
        e.delaySecs = g._beatClock->BeatTimeToSecs(_noteLength);
        for (int i = 0; i < numPlayedNotes; ++i) {
            e.midiNote = midiNotes[i]._note;
            for (int channel : _channels) {
                e.channel = channel;
                g._audioContext->AddEvent(e);
            }
        }

        if (_primePorta) {
            _primePorta = false;
        }
    } else {
        audio::Event e;
        e.delaySecs = 0.0;
        e.type = audio::EventType::PlayPcm;
        e.loop = false;
        for (int i = 0; i < numPlayedNotes; ++i) {
            e.pcmSoundIx = midiNotes[i]._note;
            e.pcmVelocity = midiNotes[i]._v * _gain;
            g._audioContext->AddEvent(e);
        }
    }
}

void StepSequencerEntity::UpdateDerived(GameManager& g, float dt) {    
    if (_permanentSequence.empty()) {
        return;
    }
    
    BeatClock const& beatClock = *g._beatClock;

    double const beatTime = beatClock.GetBeatTimeFromEpoch();

    if (_seqNeedsReset) {
        _lastPlayedNoteTime = 0.0;
        _loopStartBeatTime = g._beatClock->GetNextDownBeatTime(beatTime);
        _changeQueueHeadIx = 0;
        _changeQueueTailIx = 0;
        _changeQueueCount = 0;
        _currentIx = 0;
        _changeQueueHeadIx = 0;
        _changeQueueTailIx = 0;
        _changeQueueCount = 0;
        _primePorta = true;
        _seqNeedsReset = false;
    }

    // Play sound on the first frame at-or-after the target beat time
    double nextStepBeatTime = _loopStartBeatTime + _currentIx * _stepBeatLength;

    if (beatTime < nextStepBeatTime) {
        return;
    }

    if (_changeQueueCount > 0) {
        SeqStepChange const& change = _changeQueue[_changeQueueHeadIx];
        SeqStep& tempStep = _tempSequence[_currentIx];        
        if (change._changeNote) {
            for (int ii = 0; ii < tempStep._notes.size(); ++ii) {
                tempStep._notes[ii]._note = change._step._notes[ii]._note;
            }
        }
        if (change._changeVelocity) {
            for (int ii = 0; ii < tempStep._notes.size(); ++ii) {
                tempStep._notes[ii]._v = change._step._notes[ii]._v;
            }
        }
        for (int paramIx = 0; paramIx < kNumParamTracks; ++paramIx) {
            if (!change._step._params[paramIx]._active) {
                continue;
            }
            tempStep._params[paramIx] = change._step._params[paramIx];
        }
        if (!change._temporary) {
            _permanentSequence[_currentIx] = tempStep;
        }
        _changeQueueHeadIx = (_changeQueueHeadIx + 1) % kChangeQueueSize;
        --_changeQueueCount;
    }   

    // Play the sound
    {
        SeqStep const& seqStep = _tempSequence[_currentIx];
        PlayStep(g, seqStep);
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

void StepSequencerEntity::UpdateEditMode(GameManager& g, float dt, bool isActive) {
    if (!_hasInitEditMode) {
        BaseEntity::Init(g);
        _hasInitEditMode = true;
    }
    UpdateDerived(g, dt);
}

void StepSequencerEntity::SaveDerived(serial::Ptree pt) const {
    {
        serial::Ptree paramTrackTypePt = pt.AddChild("param_track_types");
        for (int ii = 0; ii < kNumParamTracks; ++ii) {
            serial::Ptree childPt = paramTrackTypePt.AddChild("c");
            childPt.PutBool("active", _paramTrackTypes[ii]._active);
            serial::PutEnum(childPt, "type", _paramTrackTypes[ii]._type);
        }
    }

    serial::SaveVectorInChildNode(pt, "sequence", "s", _initialMidiSequenceDoNotChange);    

    pt.PutBool("late_changes", _enableLateChanges);
    pt.PutBool("start_mute", _startMute);
    pt.PutFloat("gain", _initGain);
    
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
    pt.PutBool("quantize_temp_changes", _quantizeTempStepChanges);
}

void StepSequencerEntity::LoadDerived(serial::Ptree pt) {
    {
        serial::Ptree paramTrackTypePt = pt.TryGetChild("param_track_types");
        if (paramTrackTypePt.IsValid()) {
            int numChildren = 0;
            serial::NameTreePair* children = paramTrackTypePt.GetChildren(&numChildren);
            if (numChildren != kNumParamTracks) {
                printf("StepSequencerEntity::Load: expected %d track types, but saw %d!!!!!\n", kNumParamTracks, numChildren);
                numChildren = std::min(numChildren, kNumParamTracks);
            }
            for (int ii = 0; ii < numChildren; ++ii) {
                children[ii]._pt.TryGetBool("active", &_paramTrackTypes[ii]._active);
                serial::TryGetEnum(children[ii]._pt, "type", _paramTrackTypes[ii]._type);
            }
            delete[] children;
        }
    }

    int constexpr kLastStringSequenceVersion = 9;
    if (pt.GetVersion() <= kLastStringSequenceVersion) {
        std::string seq = pt.GetString("sequence");
        std::stringstream seqSs(seq);
        LoadSequenceFromInput(seqSs, _initialMidiSequenceDoNotChange);
    } else {
        serial::LoadVectorFromChildNode(pt, "sequence", _initialMidiSequenceDoNotChange);
    }

    _enableLateChanges = true;
    pt.TryGetBool("late_changes", &_enableLateChanges);

    pt.TryGetBool("start_mute", &_startMute);
    _initGain = 1.f;
    pt.TryGetFloat("gain", &_initGain);
    
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

    _quantizeTempStepChanges = true;
    pt.TryGetBool("quantize_temp_changes", &_quantizeTempStepChanges);
}

ne::BaseEntity::ImGuiResult StepSequencerEntity::ImGuiDerived(GameManager& g) {
    bool needsInit = false;
    ImGui::Checkbox("Mute in Editor", &_editorMute);
    ImGui::Checkbox("Start mute", &_startMute);
    if (ImGui::InputFloat("Gain", &_initGain)) {
        _gain = _initGain;
    }
    if (ImGui::InputDouble("Step length", &_stepBeatLength, ImGuiInputTextFlags_EnterReturnsTrue)) {
        needsInit = true;
    }
    ImGui::Checkbox("Is synth", &_isSynth);
    ImGui::InputDouble("Note length", &_noteLength, 0.0, 0.0, "%.6f", ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::TreeNode("Channels")) {
        imgui_util::InputVector(_channels);
        ImGui::TreePop();
    }
    
    ImGui::Checkbox("Quantize temp changes", &_quantizeTempStepChanges);
    ImGui::Checkbox("Late changes", &_enableLateChanges);
    ImGui::InputInt("Init max voices", &_initMaxNumVoices, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::InputInt("Max voices (not saved)", &_maxNumVoices, ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::TreeNode("Param track types")) {
        for (int ii = 0; ii < kNumParamTracks; ++ii) {
            ImGui::PushID(ii);
            needsInit = ImGui::Checkbox("Active", &_paramTrackTypes[ii]._active) || needsInit;
            if (_paramTrackTypes[ii]._active) {
                ImGui::SameLine();
                needsInit = audio::SynthParamTypeImGui("##Type", &_paramTrackTypes[ii]._type) || needsInit;
            }
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Sequence")) {
        needsInit = SeqImGui("Sequence", !_isSynth, _initialMidiSequenceDoNotChange) || needsInit;
        ImGui::TreePop();
    }
    if (needsInit) {
        return ImGuiResult::NeedsInit;
    } else {
        return ImGuiResult::Done;
    }
}
