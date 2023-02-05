#include "seq_action.h"

#include <sstream>

#include "entities/param_automator.h"
#include "audio.h"
#include "midi_util.h"
#include "string_util.h"
#include "seq_actions/spawn_enemy.h"
#include "seq_actions/camera_control.h"
#include "entities/typing_player.h"
#include "entities/flow_player.h"
#include "sound_bank.h"

std::unique_ptr<SeqAction> SeqAction::LoadAction(LoadInputs const& loadInputs, std::istream& input) {
    std::string token;
    input >> token;

    // TODO: get these from the SeqActionType enum
    std::unique_ptr<SeqAction> pAction;
    if (token == "automate") {
        pAction = std::make_unique<SpawnAutomatorSeqAction>();
    } else if (token == "remove_entity") {
        pAction = std::make_unique<RemoveEntitySeqAction>();
    } else if (token == "change_step") {
        pAction = std::make_unique<ChangeStepSequencerSeqAction>();
    } else if (token == "set_all_steps") {
        pAction = std::make_unique<SetAllStepsSeqAction>();
    } else if (token == "set_step_seq") {
        pAction = std::make_unique<SetStepSequenceSeqAction>();
    } else if (token == "note_on_off") {
        pAction = std::make_unique<NoteOnOffSeqAction>();
    } else if (token == "e") {
        pAction = std::make_unique<SpawnEnemySeqAction>();
    } else if (token == "b_e") {
        pAction = std::make_unique<BeatTimeEventSeqAction>();
    } else if (token == "camera") {
        pAction = std::make_unique<CameraControlSeqAction>();
    } else {
        printf("ERROR: Unrecognized action type \"%s\".\n", token.c_str());
    }

    if (pAction == nullptr) {
        return nullptr;
    }

    pAction->Load(loadInputs, input);

    return pAction;
}

void SeqAction::LoadAndInitActions(GameManager& g, std::istream& input, std::vector<BeatTimeAction>& actions) {
    double startBeatTime = 0.0;
    SeqAction::LoadInputs loadInputs;
    loadInputs._beatTimeOffset = 0.0;
    std::string line;
    std::string token;
    int nextSectionId = 0;    
    while (!input.eof()) {
        std::getline(input, line);
        // If it's empty, skip.
        if (line.empty()) {
            continue;
        }
        // If it's only whitespace, just skip.
        bool onlySpaces = true;
        for (char c : line) {
            if (!isspace(c)) {
                onlySpaces = false;
                break;
            }
        }
        if (onlySpaces) {
            continue;
        }

        // skip commented-out lines
        if (line[0] == '#') {
            continue;
        }

        std::stringstream lineStream(line);

        lineStream >> token;
        if (token == "offset") {
            lineStream >> loadInputs._beatTimeOffset;
            continue;
        } else if (token == "start") {
            lineStream >> startBeatTime;
            continue;
        } else if (token == "start_save") {
            loadInputs._defaultEnemiesSave = true;
            continue;
        } else if (token == "stop_save") {
            loadInputs._defaultEnemiesSave = false;
            continue;
        } else if (token == "section") {
            // lineStream >> loadInputs._sectionId;
            loadInputs._sectionId = nextSectionId++;
            int sectionNumBeats;
            lineStream >> sectionNumBeats;
            int numEntities = 0;
            ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::TypingPlayer, &numEntities);
            assert(numEntities == 1);
            TypingPlayerEntity* player = static_cast<TypingPlayerEntity*>(eIter.GetEntity());
            assert(player != nullptr);
            player->SetSectionLength(loadInputs._sectionId, sectionNumBeats);
            continue;
        } else if (token == "flow_section") {
            loadInputs._sectionId = nextSectionId++;
            std::string sectionDirStr;
            lineStream >> sectionDirStr;
            string_util::ToLower(sectionDirStr);
            int numEntities = 0;
            ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::FlowPlayer, &numEntities);
            assert(numEntities == 1);
            FlowPlayerEntity* player = static_cast<FlowPlayerEntity*>(eIter.GetEntity());
            assert(player != nullptr);
            FlowPlayerEntity::SectionDir sectionDir = FlowPlayerEntity::SectionDir::Up;
            if (sectionDirStr == "center") {
                sectionDir = FlowPlayerEntity::SectionDir::Center;
            } else if (sectionDirStr == "up") {
                sectionDir = FlowPlayerEntity::SectionDir::Up;
            } else if (sectionDirStr == "down") {
                sectionDir = FlowPlayerEntity::SectionDir::Down;
            } else if (sectionDirStr == "left") {
                sectionDir = FlowPlayerEntity::SectionDir::Left;
            } else if (sectionDirStr == "right") {
                sectionDir = FlowPlayerEntity::SectionDir::Right;
            } else {
                printf("ERROR: unrecognized flow_section direction \"%s\"\n", sectionDirStr.c_str());
            }
            player->_sectionDirs[loadInputs._sectionId] = sectionDir;
            continue;
        }

        double inputTime = std::stod(token);
        double beatTime = -1.0;
        if (inputTime < 0) {
            beatTime = inputTime;
        } else {
            double beatTimeAbs = inputTime + loadInputs._beatTimeOffset;
            if (beatTimeAbs < startBeatTime) {
                continue;
            }
            beatTime = beatTimeAbs - startBeatTime;
        }

        std::unique_ptr<SeqAction> pAction = SeqAction::LoadAction(loadInputs, lineStream);

        pAction->InitBase(g);

        actions.emplace_back();
        BeatTimeAction& newBta = actions.back();
        newBta._pAction = std::move(pAction);
        newBta._beatTime = beatTime;
    }

    std::stable_sort(actions.begin(), actions.end(),
                     [](BeatTimeAction const& lhs, BeatTimeAction const& rhs) {
                         return lhs._beatTime < rhs._beatTime;
                     });
}

void SpawnAutomatorSeqAction::Load(LoadInputs const& loadInputs, std::istream& input) {
    std::string paramName;
    input >> paramName;
    if (paramName == "seq_velocity") {
        _synth = false;
        input >> _seqEntityName;
    } else {
        _synth = true;
        _synthParam = audio::StringToSynthParamType(paramName.c_str());
        input >> _channel;
    }
    input >> _startValue;
    input >> _endValue;
    input >> _desiredAutomateTime;
}

void SpawnAutomatorSeqAction::Save(std::ostream& output) const {
    output << "automate" << " ";
    if (_synth) {
        output << audio::SynthParamTypeToString(_synthParam);
        output << " " << _channel;
    } else {
        output << "seq_velocity";
    }
    output << " " << _startValue;
    output << " " << _endValue;
    output << " " << _desiredAutomateTime;
}

void SpawnAutomatorSeqAction::Execute(GameManager& g) {
    ParamAutomatorEntity* e = static_cast<ParamAutomatorEntity*>(g._neEntityManager->AddEntity(ne::EntityType::ParamAutomator));
    e->_startValue = _startValue;
    e->_endValue = _endValue;
    e->_desiredAutomateTime = _desiredAutomateTime;
    e->_synth = _synth;
    e->_synthParam = _synthParam;
    e->_channel = _channel;
    e->_seqEntityName = _seqEntityName;
    e->Init(g);
}

void RemoveEntitySeqAction::Load(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityName;
}

void RemoveEntitySeqAction::Save(std::ostream& output) const {
    output << "remove_entity" << " ";
    output << _entityName;
}

void RemoveEntitySeqAction::Execute(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_entityName);
    if (e) {
        g._neEntityManager->TagForDestroy(e->_id);
    } else {
        printf("RemoveEntitySeqAction could not find entity with name \"%s\"\n", _entityName.c_str());
    }
}

void ChangeStepSequencerSeqAction::Execute(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        StepSequencerEntity::StepSaveType saveType = _temporary ? StepSequencerEntity::StepSaveType::Temporary : StepSequencerEntity::StepSaveType::Permanent;
        if (_velOnly) {
            seq->SetNextSeqStepVelocity(g, _velocity, saveType);
        } else {
            StepSequencerEntity::SeqStep step;
            step._midiNote = _midiNotes;
            step._velocity = _velocity;
            seq->SetNextSeqStep(g, std::move(step), saveType);
        }
    } else {
        printf("ChangeStepSequencerSeqAction: no seq entity!!\n");
        return;
    }
}

void ChangeStepSequencerSeqAction::Load(LoadInputs const& loadInputs, std::istream& input) {
    input >> _seqName;

    input >> _velOnly;
    input >> _temporary;
    

    StepSequencerEntity::SeqStep step;
    StepSequencerEntity::TryReadSeqStep(input, step);
    for (int i = 0; i < 4; ++i) {
        _midiNotes[i] = step._midiNote[i];
    }
    _velocity = step._velocity;
}

void ChangeStepSequencerSeqAction::Save(std::ostream& output) const {
    output << "change_step" << " ";
    output << _seqName;
    output << " " << _velOnly;
    output << " " << _temporary;
    output << " ";
    StepSequencerEntity::SeqStep step;
    step._midiNote = _midiNotes;
    step._velocity = _velocity;
    StepSequencerEntity::WriteSeqStep(step, output);
}

void ChangeStepSequencerSeqAction::Init(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_seqName);
    if (e) {
        _seqId = e->_id;
    } else {
        printf("ChangeStepSequencerSeqAction: could not find seq entity \"%s\"\n", _seqName.c_str());
    }
}

void SetAllStepsSeqAction::Load(LoadInputs const& loadInputs, std::istream& input) {
    input >> _seqName;
    assert(_midiNotes.size() == 4);
    input >> _midiNotes[0] >> _midiNotes[1] >> _midiNotes[2] >> _midiNotes[3];
    input >> _velocity;
}

void SetAllStepsSeqAction::Init(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_seqName);
    if (e) {
        _seqId = e->_id;
    } else {
        printf("SetAllStepsSeqAction: could not find seq entity \"%s\"\n", _seqName.c_str());
    }
}

void SetAllStepsSeqAction::Execute(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        StepSequencerEntity::SeqStep step;
        step._midiNote = _midiNotes;
        step._velocity = _velocity;
        seq->SetAllStepsPermanent(step);               
    } else {
        printf("SetAllStepsSeqAction: no seq entity!!\n");
        return;
    }
}

void SetStepSequenceSeqAction::Load(LoadInputs const& loadInputs, std::istream& input) {
    input >> _seqName;   
    StepSequencerEntity::LoadSequenceFromInput(input, _sequence);
}

void SetStepSequenceSeqAction::Init(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_seqName);
    if (e) {
        _seqId = e->_id;
    } else {
        printf("SetStepSequenceSeqAction: could not find seq entity \"%s\"\n", _seqName.c_str());
    }
}

void SetStepSequenceSeqAction::Execute(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        seq->SetSequencePermanent(_sequence);               
    } else {
        printf("SetStepSequenceSeqAction: no seq entity!!\n");
        return;
    }
}

void NoteOnOffSeqAction::Execute(GameManager& g) {
    BeatTimeEvent b_e;
    b_e._beatTime = 0.0;
    b_e._e.type = audio::EventType::NoteOn;
    b_e._e.channel = _channel;
    b_e._e.midiNote = _midiNote;
    b_e._e.velocity =_velocity;
                
    audio::Event e = GetEventAtBeatOffsetFromNextDenom(_quantizeDenom, b_e, *g._beatClock, /*slack=*/0.0625);
    g._audioContext->AddEvent(e);

    b_e._e.type = audio::EventType::NoteOff;
    b_e._beatTime = _noteLength;
    e = GetEventAtBeatOffsetFromNextDenom(_quantizeDenom, b_e, *g._beatClock, /*slack=*/0.0625);
    g._audioContext->AddEvent(e);
}

void NoteOnOffSeqAction::Load(LoadInputs const& loadInputs, std::istream& input) {
    std::string token, key, value;
    while (!input.eof()) {
        {
            input >> token;
            std::size_t delimIx = token.find_first_of(':');
            if (delimIx == std::string::npos) {
                printf("Token missing \":\" - \"%s\"\n", token.c_str());
                continue;
            }

            key = token.substr(0, delimIx);
            value = token.substr(delimIx+1);
        }
        if (key == "channel") {
            _channel = std::stoi(value);
        } else if (key == "note") {
            _midiNote = GetMidiNote(value.c_str());
        } else if (key == "length") {
            _noteLength = std::stod(value);
        } else if (key == "velocity") {
            _velocity = std::stof(value);
        } else if (key == "quantize") {
            _quantizeDenom = std::stod(value);
        } else {
            printf("NoteOnOffSeqAction::Load: unknown key \"%s\"\n", key.c_str());
        }
    }
}

void BeatTimeEventSeqAction::Execute(GameManager& g) {
    audio::Event e;
    if (_quantizeDenom >= 0) {
        e = GetEventAtBeatOffsetFromNextDenom(_quantizeDenom, _b_e, *g._beatClock, /*slack=*/0.0625);
    } else {
        e = _b_e._e;
        e.timeInTicks = g._beatClock->EpochBeatTimeToTickTime(_b_e._beatTime);
    }
    g._audioContext->AddEvent(e);
}

void BeatTimeEventSeqAction::Load(LoadInputs const& loadInputs, std::istream& input) {
    // quantize denom first
    input >> _quantizeDenom;
    ReadBeatEventFromTextLineNoSoundLookup(input, _b_e, _soundBankName);
    _b_e._beatTime += loadInputs._beatTimeOffset;   
}

void BeatTimeEventSeqAction::Init(GameManager& g) {
    if (!_soundBankName.empty()) {
        int soundIx = g._soundBank->GetSoundIx(_soundBankName.c_str());
        if (soundIx < 0) {
            printf("Failed to find sound \"%s\"\n", _soundBankName.c_str());   
        } else {
            _b_e._e.pcmSoundIx = soundIx;
        }
    }
}
