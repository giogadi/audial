#include "seq_action.h"

#include <sstream>

#include "imgui/imgui.h"

#include "entities/param_automator.h"
#include "audio.h"
#include "midi_util.h"
#include "string_util.h"
#include "seq_actions/spawn_enemy.h"
#include "seq_actions/camera_control.h"
#include "entities/typing_player.h"
#include "entities/flow_player.h"
#include "entities/flow_wall.h"
#include "entities/int_variable.h"
#include "sound_bank.h"
#include "imgui_util.h"

#include "enums/SeqActionType.h"

void SeqAction::Save(serial::Ptree pt) const {
    pt.PutBool("one_time", _oneTime);
    pt.PutString("action_type", SeqActionTypeToString(Type()));
    SaveDerived(pt);
}

void SeqAction::SaveActionsInChildNode(serial::Ptree pt, char const* childName, std::vector<std::unique_ptr<SeqAction>> const& actions) {
    serial::Ptree vecPt = pt.AddChild(childName);
    for (auto const& action : actions) {
        serial::Ptree childPt = vecPt.AddChild("seq_action");
        action->Save(childPt);
    }
}

std::unique_ptr<SeqAction> SeqAction::New(SeqActionType actionType) {
    switch (actionType) {
        case SeqActionType::SpawnAutomator: return std::make_unique<SpawnAutomatorSeqAction>();
        case SeqActionType::RemoveEntity: return std::make_unique<RemoveEntitySeqAction>();
        case SeqActionType::ChangeStepSequencer: return std::make_unique<ChangeStepSequencerSeqAction>();
        case SeqActionType::SetAllSteps: return std::make_unique<SetAllStepsSeqAction>();
        case SeqActionType::SetStepSequence: return std::make_unique<SetStepSequenceSeqAction>();
        case SeqActionType::SetStepSequencerMute: return std::make_unique<SetStepSequencerMuteSeqAction>();
        case SeqActionType::NoteOnOff: return std::make_unique<NoteOnOffSeqAction>();
        case SeqActionType::BeatTimeEvent: return std::make_unique<BeatTimeEventSeqAction>();
        case SeqActionType::WaypointControl: return std::make_unique<WaypointControlSeqAction>();
        case SeqActionType::PlayerSetKillZone: return std::make_unique<PlayerSetKillZoneSeqAction>();
        case SeqActionType::PlayerSetSpawnPoint: return std::make_unique<PlayerSetSpawnPointSeqAction>();
        case SeqActionType::SetNewFlowSection: return std::make_unique<SetNewFlowSectionSeqAction>();
        case SeqActionType::AddToIntVariable: return std::make_unique<AddToIntVariableSeqAction>();
        case SeqActionType::CameraControl: return std::make_unique<CameraControlSeqAction>();
        case SeqActionType::SpawnEnemy: return std::make_unique<SpawnEnemySeqAction>();
        case SeqActionType::Count: break;
    }
    assert(false);
    return nullptr;
}

std::unique_ptr<SeqAction> SeqAction::Load(serial::Ptree pt) {
    SeqActionType actionType;
    bool found = TryGetEnum(pt, "action_type", actionType);
    if (!found) {
        printf("Could not find action_type!\n");
        return nullptr;
    }
    std::unique_ptr<SeqAction> pAction = SeqAction::New(actionType);
    pt.TryGetBool("one_time", &(pAction->_oneTime));
    pAction->LoadDerived(pt);
    return pAction;
}

void SeqAction::LoadActionsFromChildNode(serial::Ptree pt, char const* childName, std::vector<std::unique_ptr<SeqAction>>& actions) {
    actions.clear();
    serial::Ptree actionsPt = pt.TryGetChild(childName);
    if (!actionsPt.IsValid()) {
        printf("LoadActionsFromChildNode: no child found of name \"%s\"!\n", childName);
        return;
    }
    int numChildren;
    serial::NameTreePair* children = actionsPt.GetChildren(&numChildren);
    actions.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        actions.push_back(Load(children[i]._pt));
    }
    delete[] children;
}

bool SeqAction::ImGui(char const* label, std::vector<std::unique_ptr<SeqAction>>& actions) {
    bool changed = false;
    if (ImGui::CollapsingHeader(label)) {
        ImGui::PushID(label);
        
        static SeqActionType actionType = SeqActionType::SpawnAutomator;
        SeqActionTypeImGui("Action type", &actionType);
        if (ImGui::Button("Add")) {
            actions.push_back(SeqAction::New(actionType));
            changed = true;
        }        
        int deleteIx = -1;
        for (int i = 0, n = actions.size(); i < n; ++i) {
            ImGui::PushID(i);
            if (ImGui::CollapsingHeader(SeqActionTypeToString(actions[i]->Type()))) {
                bool thisChanged = ImGui::Checkbox("One time", &actions[i]->_oneTime);
                changed = thisChanged || changed;

                thisChanged = actions[i]->ImGui();
                changed = thisChanged || changed;
                if (ImGui::Button("Remove")) {
                    deleteIx = i;
                    changed = true;
                }
            }
            ImGui::PopID();
        }
        if (deleteIx >= 0) {
            actions.erase(actions.begin() + deleteIx);
        }
        
        ImGui::PopID();
    }
    
    return changed;
}

std::unique_ptr<SeqAction> SeqAction::LoadAction(LoadInputs const& loadInputs, std::istream& input) {
    std::string token;
    input >> token;

    bool oneTime = false;
    if (token == "onetime") {
        oneTime = true;
        input >> token;
    }

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
    } else if (token == "step_seq_mute") {
        pAction = std::make_unique<SetStepSequencerMuteSeqAction>();
    } else if (token == "note_on_off") {
        pAction = std::make_unique<NoteOnOffSeqAction>();
    } else if (token == "e") {
        pAction = std::make_unique<SpawnEnemySeqAction>();
    } else if (token == "b_e") {
        pAction = std::make_unique<BeatTimeEventSeqAction>();
    } else if (token == "camera") {
        pAction = std::make_unique<CameraControlSeqAction>();
    } else if (token == "waypoint") {
        pAction = std::make_unique<WaypointControlSeqAction>();
    } else if (token == "player_killzone") {
        pAction = std::make_unique<PlayerSetKillZoneSeqAction>();
    } else if (token == "set_new_section") {
        pAction = std::make_unique<SetNewFlowSectionSeqAction>();
    } else if (token == "set_player_spawn") {
        pAction = std::make_unique<PlayerSetSpawnPointSeqAction>();
    } else if (token == "add_int") {
        pAction = std::make_unique<AddToIntVariableSeqAction>();
    } else {
        printf("ERROR: Unrecognized action type \"%s\".\n", token.c_str());
    }

    if (pAction == nullptr) {
        return nullptr;
    }

    pAction->_oneTime = oneTime;

    pAction->LoadDerived(loadInputs, input);

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

        pAction->Init(g);

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

void SpawnAutomatorSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    std::string paramName;
    std::string token;
    input >> token;
    if (token == "relative") {
        _props._relative = true;
        input >> paramName;
    } else {
        paramName = token;
    }
    if (paramName == "seq_velocity") {
        _props._synth = false;
        input >> _props._seqEntityName;
    } else {
        _props._synth = true;
        _props._synthParam = audio::StringToSynthParamType(paramName.c_str());
        input >> _props._channel;
    }
    input >> _props._startValue;
    input >> _props._endValue;
    input >> _props._desiredAutomateTime;
}

void SpawnAutomatorSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    ParamAutomatorEntity* e = static_cast<ParamAutomatorEntity*>(g._neEntityManager->AddEntity(ne::EntityType::ParamAutomator));
    e->_relative = _props._relative;
    e->_startValue = _props._startValue;
    e->_endValue = _props._endValue;
    e->_desiredAutomateTime = _props._desiredAutomateTime;
    e->_synth = _props._synth;
    e->_synthParam = _props._synthParam;
    e->_channel = _props._channel;
    e->_seqEntityName = _props._seqEntityName;
    e->Init(g);
}

void RemoveEntitySeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityName;
}

void RemoveEntitySeqAction::LoadDerived(serial::Ptree pt) {
    _entityName = pt.GetString("entity_name");
}
void RemoveEntitySeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutString("entity_name", _entityName.c_str());
}
bool RemoveEntitySeqAction::ImGui() {
    return imgui_util::InputText<128>("Entity name", &_entityName);
}

void RemoveEntitySeqAction::ExecuteDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_entityName);
    if (e) {
        g._neEntityManager->TagForDestroy(e->_id);
    } else {
        printf("RemoveEntitySeqAction could not find entity with name \"%s\"\n", _entityName.c_str());
    }
}

void ChangeStepSequencerSeqAction::ExecuteDerived(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        StepSequencerEntity::StepSaveType saveType;
        if (g._editMode) {
            saveType = StepSequencerEntity::StepSaveType::Temporary;
        } else {
            saveType = _props._temporary ? StepSequencerEntity::StepSaveType::Temporary : StepSequencerEntity::StepSaveType::Permanent;
        }        
        
        if (_props._velOnly) {
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

void ChangeStepSequencerSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _props._seqName;

    input >> _props._velOnly;
    input >> _props._temporary;
    
    std::getline(input, _props._stepStr);
}

void ChangeStepSequencerSeqAction::LoadDerived(serial::Ptree pt) {
    _props.Load(pt);
}

void ChangeStepSequencerSeqAction::SaveDerived(serial::Ptree pt) const {
    _props.Save(pt);
}

bool ChangeStepSequencerSeqAction::ImGui() {
    return _props.ImGui();
}

void ChangeStepSequencerSeqAction::InitDerived(GameManager& g) {        
    ne::Entity* e = g._neEntityManager->FindEntityByName(_props._seqName);
    if (e) {
        _seqId = e->_id;
    } else {
        printf("ChangeStepSequencerSeqAction: could not find seq entity \"%s\"\n", _props._seqName.c_str());
    }

    std::stringstream ss(_props._stepStr);
    StepSequencerEntity::SeqStep step;
    if (!StepSequencerEntity::TryReadSeqStep(ss, step)) {
        printf("ChangeStepSequencerSeqAction: failed to parse step \"%s\"\n", _props._stepStr.c_str());
    }
    for (int i = 0; i < 4; ++i) {
        _midiNotes[i] = step._midiNote[i];
    }
    _velocity = step._velocity;
}

void SetAllStepsSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _seqName;
    input >> _velOnly;

    std::getline(input, _stepStr);
    
    // std::string token;
    // input >> token;
    // if (token == "vel_only") {
    //     _velOnly = true;
    //     input >> _seqName;
    // } else {
    //     _seqName = token;
    // }
    // if (!_velOnly) {
    //     assert(_midiNotes.size() == 4);
    //     input >> _midiNotes[0] >> _midiNotes[1] >> _midiNotes[2] >> _midiNotes[3];
    // }
    // input >> _velocity;
}
void SetAllStepsSeqAction::LoadDerived(serial::Ptree pt) {
    _seqName = pt.GetString("seq_name");
    _stepStr = pt.GetString("step_str");
    _velOnly = pt.GetBool("vel_only");
}
void SetAllStepsSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutString("seq_name", _seqName.c_str());
    pt.PutString("step_str", _stepStr.c_str());
    pt.PutBool("vel_only", _velOnly);
}
bool SetAllStepsSeqAction::ImGui() {
    imgui_util::InputText<128>("Seq entity name", &_seqName);
    imgui_util::InputText<128>("Step string", &_stepStr);
    ImGui::Checkbox("Velocity only", &_velOnly);
    return false;
}

void SetAllStepsSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_seqName);
    if (e) {
        _seqId = e->_id;
    } else {
        printf("SetAllStepsSeqAction: could not find seq entity \"%s\"\n", _seqName.c_str());
    }

    std::stringstream ss(_stepStr);
    StepSequencerEntity::SeqStep step;
    if (!StepSequencerEntity::TryReadSeqStep(ss, step)) {
        printf("SetAllStepsSeqAction: failed to parse step \"%s\"\n", _stepStr.c_str());
    }
    for (int i = 0; i < 4; ++i) {
        _midiNotes[i] = step._midiNote[i];
    }
    _velocity = step._velocity;
}

void SetAllStepsSeqAction::ExecuteDerived(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        if (_velOnly) {
            seq->SetAllVelocitiesPermanent(_velocity);
        } else {
            StepSequencerEntity::SeqStep step;
            if (!_velOnly) {
                step._midiNote = _midiNotes;
            }
            step._velocity = _velocity;
            seq->SetAllStepsPermanent(step);
        }
    } else {
        printf("SetAllStepsSeqAction: no seq entity!!\n");
        return;
    }
}

void SetStepSequenceSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _seqName;   
    StepSequencerEntity::LoadSequenceFromInput(input, _sequence);
}

void SetStepSequenceSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_seqName);
    if (e) {
        _seqId = e->_id;
    } else {
        printf("SetStepSequenceSeqAction: could not find seq entity \"%s\"\n", _seqName.c_str());
    }
}

void SetStepSequenceSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        seq->SetSequencePermanent(_sequence);               
    } else {
        printf("SetStepSequenceSeqAction: no seq entity!!\n");
        return;
    }
}

void NoteOnOffSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
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
            _props._channel = std::stoi(value);
        } else if (key == "note") {
            _props._midiNote = GetMidiNote(value.c_str());
        } else if (key == "length") {
            _props._noteLength = std::stod(value);
        } else if (key == "velocity") {
            _props._velocity = std::stof(value);
        } else if (key == "quantize") {
            _props._quantizeDenom = std::stod(value);
        } else {
            printf("NoteOnOffSeqAction::Load: unknown key \"%s\"\n", key.c_str());
        }
    }
}

void NoteOnOffSeqAction::ExecuteDerived(GameManager& g) {
    BeatTimeEvent b_e;
    b_e._beatTime = 0.0;
    b_e._e.type = audio::EventType::NoteOn;
    b_e._e.channel = _props._channel;
    b_e._e.midiNote = _props._midiNote;
    b_e._e.velocity = _props._velocity;
                
    audio::Event e = GetEventAtBeatOffsetFromNextDenom(_props._quantizeDenom, b_e, *g._beatClock, /*slack=*/0.0625);
    g._audioContext->AddEvent(e);

    b_e._e.type = audio::EventType::NoteOff;
    b_e._beatTime = _props._noteLength;
    e = GetEventAtBeatOffsetFromNextDenom(_props._quantizeDenom, b_e, *g._beatClock, /*slack=*/0.0625);
    g._audioContext->AddEvent(e);
}

void SetStepSequencerMuteSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _seqName;
    input >> _mute;
}

void SetStepSequencerMuteSeqAction::LoadDerived(serial::Ptree pt) {
    _seqName = pt.GetString("seq_name");
    _mute = pt.GetBool("mute");
}

void SetStepSequencerMuteSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutString("seq_name", _seqName.c_str());
    pt.PutBool("mute", _mute);
}

bool SetStepSequencerMuteSeqAction::ImGui() {
    imgui_util::InputText<128>("Seq entity name", &_seqName);
    ImGui::Checkbox("Mute", &_mute);
    return false;
}

void SetStepSequencerMuteSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_seqName);
    if (e) {
        _seqId = e->_id;
    } else {
        printf("SetStepSequenceMuteSeqAction: could not find seq entity \"%s\"\n", _seqName.c_str());
    }
}

void SetStepSequencerMuteSeqAction::ExecuteDerived(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        seq->_mute = _mute;               
    } else {
        printf("SetStepSequenceMuteSeqAction: no seq entity!!\n");
        return;
    }
}

void BeatTimeEventSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    // quantize denom first
    input >> _quantizeDenom;
    std::string soundBankName;
    ReadBeatEventFromTextLineNoSoundLookup(input, _b_e, soundBankName);
    if (!soundBankName.empty()) {
        printf("BeatTimeEventSeqAction::LoadDerived: WARNING, we no longer support sound bank names when loading from text lines!\n");        
    }
    _b_e._beatTime += loadInputs._beatTimeOffset;
}

void BeatTimeEventSeqAction::LoadDerived(serial::Ptree pt) {
    _quantizeDenom = pt.GetDouble("quantize_denom");
    serial::LoadFromChildOf(pt, "beat_time_event", _b_e);
}

void BeatTimeEventSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutDouble("quantize_denom", _quantizeDenom);
    serial::SaveInNewChildOf(pt, "beat_time_event", _b_e);
}

bool BeatTimeEventSeqAction::ImGui() {
    ImGui::InputScalar("Denom", ImGuiDataType_Double, &_quantizeDenom);
    _b_e.ImGui();
    return false;
}

void BeatTimeEventSeqAction::ExecuteDerived(GameManager& g) {
    audio::Event e;
    if (_quantizeDenom >= 0) {
        e = GetEventAtBeatOffsetFromNextDenom(_quantizeDenom, _b_e, *g._beatClock, /*slack=*/0.0625);
    } else {
        e = _b_e._e;
        e.timeInTicks = g._beatClock->EpochBeatTimeToTickTime(_b_e._beatTime);
    }
    g._audioContext->AddEvent(e);
}

void WaypointControlSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityName;
    input >> _followWaypoints;
}

void WaypointControlSeqAction::LoadDerived(serial::Ptree pt) {
    _followWaypoints = pt.GetBool("follow_waypoints");
    _entityName = pt.GetString("entity_name");
}

void WaypointControlSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("follow_waypoints", _followWaypoints);
    pt.PutString("entity_name", _entityName.c_str());
}

bool WaypointControlSeqAction::ImGui() {
    ImGui::Checkbox("Follow", &_followWaypoints);
    imgui_util::InputText<128>("Entity name", &_entityName);
    return false;
}

void WaypointControlSeqAction::ExecuteDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_entityName);
    if (e == nullptr) {
        printf("ERROR: WaypointControlSeqAction could not find entity \"%s\"\n", _entityName.c_str());
        return;
    }
    WaypointFollower* wpFollower = nullptr;
    if (e->_id._type == ne::EntityType::TypingEnemy) {
        TypingEnemyEntity* enemy = static_cast<TypingEnemyEntity*>(e);
        wpFollower = &enemy->_waypointFollower;
    } else if (e->_id._type == ne::EntityType::FlowWall) {
        FlowWallEntity* wall = static_cast<FlowWallEntity*>(e);
        wpFollower = &wall->_wpFollower;
    }
    
    if (wpFollower == nullptr) {
        printf("ERROR: WaypointControlSeqAction given an unsupported entity \"%s\"\n", _entityName.c_str());
        return;
    }
    // wpFollower->_followingWaypoints = _followWaypoints;
    if (_followWaypoints) {
        wpFollower->Start(g, *e);
    } else {
        wpFollower->Stop();
    }
}

void PlayerSetKillZoneSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    _maxZ.reset();

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
            value = token.substr(delimIx + 1);
        }
        if (key == "max_z") {
            _maxZ = string_util::MaybeStof(value);      
            if (!_maxZ.has_value()) {
                printf("PlayerSetKillZoneSeqAction::Load: could not get max_z value from \"%s\"\n", value.c_str());
            }
        }
        else {
            printf("PlayerSetKillZoneSeqAction::Load: unknown key \"%s\"\n", key.c_str());
        }
    }
}

void PlayerSetKillZoneSeqAction::LoadDerived(serial::Ptree pt) {
    _maxZ = std::nullopt;
    float maxZ = 0.f;
    if (pt.TryGetFloat("max_z", &maxZ)) {
        _maxZ = maxZ;
    }
}

void PlayerSetKillZoneSeqAction::SaveDerived(serial::Ptree pt) const {
    if (_maxZ.has_value()) {
        pt.PutFloat("max_z", _maxZ.value());
    }
}

bool PlayerSetKillZoneSeqAction::ImGui() {
    bool hasMaxZ = _maxZ.has_value();
    ImGui::Checkbox("Has Max Z", &hasMaxZ);
    if (hasMaxZ) {
        float maxZ = 0.f;
        if (_maxZ.has_value()) {
            maxZ = _maxZ.value();
        }
        ImGui::InputScalar("Max Z", ImGuiDataType_Float, &maxZ);
        _maxZ = maxZ;
    } else {
        _maxZ = std::nullopt;
    }
    return false;
}

void PlayerSetKillZoneSeqAction::ExecuteDerived(GameManager& g) {
    FlowPlayerEntity* pPlayer = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    pPlayer->_killMaxZ = _maxZ;
}

void SetNewFlowSectionSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    std::string token;
    input >> token;
    bool success = string_util::MaybeStoi(std::string_view(token), _newSectionId);
    if (!success) {
        printf("SetNewFlowSectionSeqAction: bad section id \"%s\"\n", token.c_str());
    }
}

void SetNewFlowSectionSeqAction::LoadDerived(serial::Ptree pt) {
    _newSectionId = pt.GetInt("section_id");
}

void SetNewFlowSectionSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("section_id", _newSectionId);
}

bool SetNewFlowSectionSeqAction::ImGui() {
    ImGui::InputInt("Section ID", &_newSectionId);
    return false;
}

void SetNewFlowSectionSeqAction::ExecuteDerived(GameManager& g) {
    g._neEntityManager->TagAllPrevSectionEntitiesForDestroy(_newSectionId);
    FlowPlayerEntity* pPlayer = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    assert(pPlayer);
    pPlayer->SetNewSection(g, _newSectionId);
}

void PlayerSetSpawnPointSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    try {
        input >> _spawnPos._x >> _spawnPos._y >> _spawnPos._z;
    } catch (std::exception&) {
        printf("PlayerSetSpawnPointSeqAction: could not parse spawn pos.\n");
    }   
}

void PlayerSetSpawnPointSeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "spawn_pos", _spawnPos);    
}

void PlayerSetSpawnPointSeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "spawn_pos", _spawnPos);
}

bool PlayerSetSpawnPointSeqAction::ImGui() {
    imgui_util::InputVec3("Spawn pos", &_spawnPos);
    return false;   
}

void PlayerSetSpawnPointSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) { return; }
    FlowPlayerEntity* pPlayer = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    pPlayer->_respawnPos = _spawnPos;
}

void AddToIntVariableSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    try {
        input >> _varName >> _addAmount;
    } catch (std::exception&) {
        printf("AddToIntVariableSeqAction::Load: could not parse input\n");
    }
}

void AddToIntVariableSeqAction::LoadDerived(serial::Ptree pt) {
    _varName = pt.GetString("var_name");
    _addAmount = pt.GetInt("add_amount");
}

void AddToIntVariableSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutString("var_name", _varName.c_str());
    pt.PutInt("add_amount", _addAmount);
}

bool AddToIntVariableSeqAction::ImGui() {
    imgui_util::InputText<128>("Var entity name", &_varName);
    ImGui::InputInt("Add amount", &_addAmount);
    return false;
}

void AddToIntVariableSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    IntVariableEntity* e = static_cast<IntVariableEntity*>(g._neEntityManager->FindEntityByNameAndType(_varName, ne::EntityType::IntVariable));
    if (e == nullptr) {
        printf("AddToIntVariableSeqAction::ExecuteDerived: couldn't entity \"%s\"!\n", _varName.c_str());
    } else {        
        e->AddToVariable(_addAmount);
    }
}
