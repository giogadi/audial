#include "actions.h"

#include <sstream>

#include "entities/param_automator.h"
#include "audio.h"
#include "midi_util.h"
#include "string_util.h"
#include "entities/flow_player.h"
#include "entities/flow_wall.h"
#include "entities/int_variable.h"
#include "entities/flow_trigger.h"
#include "entities/typing_enemy.h"
#include "sound_bank.h"
#include "imgui_util.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"

extern GameManager gGameManager;

void SpawnAutomatorSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    std::string paramName;
    std::string token;
    input >> token;
    if (token == "relative") {
        _props._relative = true;
        input >> paramName;
    }
    else {
        paramName = token;
    }
    if (paramName == "seq_velocity") {
        _props._synth = false;
        input >> _props._seqEditorId;
    }
    else {
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
    e->_props = _props;
    e->Init(g);
}

void RemoveEntitySeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityEditorId;
}

void RemoveEntitySeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "entity_editor_id", _entityEditorId);
}
void RemoveEntitySeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "entity_editor_id", _entityEditorId);
}
bool RemoveEntitySeqAction::ImGui() {
    return imgui_util::InputEditorId("Entity ID", &_entityEditorId);
}

void RemoveEntitySeqAction::ExecuteDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByEditorId(_entityEditorId, nullptr, "RemoveEntitySeqAction");
    if (e) {
        g._neEntityManager->TagForDestroy(e->_id);
    }
}

void ChangeStepSequencerSeqAction::ExecuteDerived(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        StepSequencerEntity::StepSaveType saveType;
        if (g._editMode) {
            saveType = StepSequencerEntity::StepSaveType::Temporary;
        }
        else {
            saveType = _props._temporary ? StepSequencerEntity::StepSaveType::Temporary : StepSequencerEntity::StepSaveType::Permanent;
        }

        StepSequencerEntity::SeqStep step;
        for (int i = 0; i < StepSequencerEntity::SeqStep::kNumNotes && i < _props._midiNotes.size(); ++i) {
            step._midiNote[i] = _props._midiNotes[i]._note;
        }
        for (int i = 0; i < StepSequencerEntity::kNumParamTracks && i < _props._params.size(); ++i) {
            step._params[i] = _props._params[i];
        }
        step._velocity = _props._velocity;
        seq->SetNextSeqStep(g, std::move(step), saveType, _props._changeNote, _props._changeVel);
    }
    else {
        printf("ChangeStepSequencerSeqAction: no seq entity!!\n");
        return;
    }
}

void ChangeStepSequencerSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    assert(false); // TODO
    // input >> _props._seqEntityEditorId;

    // input >> _props._velOnly;
    // input >> _props._temporary;

    // std::getline(input, _props._stepStr);
}

void ChangeStepSequencerSeqAction::LoadDerived(serial::Ptree pt) {
    {
        serial::LoadFromChildOf(pt, "seqEntityEditorId", _props._seqEntityEditorId);

        pt.TryGetBool("changeVel", &_props._changeVel);

        pt.TryGetBool("changeNote", &_props._changeNote);

        pt.TryGetBool("temporary", &_props._temporary);

        serial::LoadVectorFromChildNode<MidiNoteAndName>(pt, "midiNotes", _props._midiNotes);

        pt.TryGetFloat("velocity", &_props._velocity);

        serial::LoadVectorFromChildNode<StepSequencerEntity::SynthParamValue>(pt, "params", _props._params);
    }
    int constexpr kUseIntsForMidiNotesVersion = 4;
    if (pt.GetVersion() < kUseIntsForMidiNotesVersion) {
        if (!_props._midiNotes.empty()) {
            printf("ChangeStepSequencerSeqAction::LoadDerived:  UNEXPECTED NOTES IN THIS VERSION!\n");
            _props._midiNotes.clear();
        }
        std::string stepStr;
        pt.TryGetString("stepStr", &stepStr);
        std::stringstream ss(stepStr);
        StepSequencerEntity::SeqStep step;
        if (!StepSequencerEntity::TryReadSeqStep(ss, step)) {
            printf("ChangeStepSequencerSeqAction: failed to parse step \"%s\"\n", stepStr.c_str());
        }
        for (int i = 0; i < StepSequencerEntity::SeqStep::kNumNotes; ++i) {
            if (step._midiNote[i] < 0) {
                continue;
            }
            _props._midiNotes.emplace_back(step._midiNote[i]);
        }
        _props._velocity = step._velocity;
    }

    if (_props._midiNotes.size() > StepSequencerEntity::SeqStep::kNumNotes) {
        printf("ChangeStepSequencerSeqAction::LoadDerived: too many notes (%zu)!\n", _props._midiNotes.size());
    }

    int constexpr kLastWithVelOnlyVersion = 10;
    if (pt.GetVersion() <= kLastWithVelOnlyVersion) {
        bool velOnly = false;
        pt.TryGetBool("velOnly", &velOnly);
        if (velOnly) {
            _props._changeVel = true;
            _props._changeNote = false;
        } else {
            _props._changeVel = true;
            _props._changeNote = true;
        }
    }
}

void ChangeStepSequencerSeqAction::SaveDerived(serial::Ptree pt) const {
    {
        serial::SaveInNewChildOf(pt, "seqEntityEditorId", _props._seqEntityEditorId);

        pt.PutBool("changeVel", _props._changeVel);

        pt.PutBool("changeNote", _props._changeNote);

        pt.PutBool("temporary", _props._temporary);

        serial::SaveVectorInChildNode<MidiNoteAndName>(pt, "midiNotes", "array_item", _props._midiNotes);

        pt.PutFloat("velocity", _props._velocity);

        serial::SaveVectorInChildNode<StepSequencerEntity::SynthParamValue>(pt, "params", "array_item", _props._params);
    }
}

bool ChangeStepSequencerSeqAction::ImGui() {
    bool changed = false;

    changed = imgui_util::InputEditorId("seqEntityEditorId", &_props._seqEntityEditorId) || changed;

    changed = ImGui::Checkbox("changeVel", &_props._changeVel) || changed;

    changed = ImGui::Checkbox("changeNote", &_props._changeNote) || changed;

    changed = ImGui::Checkbox("temporary", &_props._temporary) || changed;

    {
        imgui_util::InputVectorOptions options;
        options.removeOnSameLine = true;
        if (ImGui::TreeNode("midiNotes")) {

            changed = imgui_util::InputVector(_props._midiNotes, options) || changed;
            ImGui::TreePop();

        }
    }

    changed = ImGui::InputFloat("velocity", &_props._velocity) || changed;

    {
        imgui_util::InputVectorOptions options;
        if (ImGui::TreeNode("params")) {
            changed = imgui_util::InputVector(_props._params, options) || changed;
            ImGui::TreePop();
        }
    }

    return changed;
}

void ChangeStepSequencerSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByEditorId(_props._seqEntityEditorId, nullptr, "ChangeStepSequencerSeqAction");
    if (e) {
        _seqId = e->_id;
    }
}

void SetAllStepsSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _props._seqEntityEditorId._id;
    input >> _props._velOnly;
    std::string stepStr;
    std::getline(input, stepStr);
    std::stringstream ss(stepStr);
    StepSequencerEntity::SeqStep step;
    if (!StepSequencerEntity::TryReadSeqStep(ss, step)) {
        printf("SetAllStepsSeqAction: failed to parse step \"%s\"\n", stepStr.c_str());
    }
    for (int i = 0; i < StepSequencerEntity::SeqStep::kNumNotes; ++i) {
        if (step._midiNote[i] < 0) {
            continue;
        }
        _props._midiNotes.emplace_back(step._midiNote[i]);
    }
    _props._velocity = step._velocity;
}
void SetAllStepsSeqAction::LoadDerived(serial::Ptree pt) {
    _props.Load(pt);
    int constexpr kUseIntsForMidiNotesVersion = 4;
    if (pt.GetVersion() < kUseIntsForMidiNotesVersion) {
        pt.TryGetBool("vel_only", &_props._velOnly);
        serial::LoadFromChildOf(pt, "seq_editor_id", _props._seqEntityEditorId);
        if (!_props._midiNotes.empty()) {
            printf("SetAllStepsSeqAction::LoadDerived:  UNEXPECTED NOTES IN THIS VERSION!\n");
            _props._midiNotes.clear();
        }
        std::string stepStr;
        pt.TryGetString("step_str", &stepStr);
        std::stringstream ss(stepStr);
        StepSequencerEntity::SeqStep step;
        if (!StepSequencerEntity::TryReadSeqStep(ss, step)) {
            printf("SetAllStepsSeqAction: failed to parse step \"%s\"\n", stepStr.c_str());
        }
        for (int i = 0; i < StepSequencerEntity::SeqStep::kNumNotes; ++i) {
            if (step._midiNote[i] < 0) {
                continue;
            }
            _props._midiNotes.emplace_back(step._midiNote[i]);
        }
        _props._velocity = step._velocity;
    }

    if (_props._midiNotes.size() > StepSequencerEntity::SeqStep::kNumNotes) {
        printf("SetAllStepsSeqAction::LoadDerived: too many notes (%zu)!\n", _props._midiNotes.size());
    }
}
void SetAllStepsSeqAction::SaveDerived(serial::Ptree pt) const {
    _props.Save(pt);
}
bool SetAllStepsSeqAction::ImGui() {
    return _props.ImGui();
}

void SetAllStepsSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_props._seqEntityEditorId, ne::EntityType::StepSequencer, nullptr, "SetAllStepsSeqAction");
    if (e) {
        _seqId = e->_id;
    }
}

void SetAllStepsSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        if (_props._velOnly) {
            seq->SetAllVelocitiesPermanent(_props._velocity);
        }
        else {
            StepSequencerEntity::SeqStep step;
            for (int i = 0; i < StepSequencerEntity::SeqStep::kNumNotes && i < _props._midiNotes.size(); ++i) {
                step._midiNote[i] = _props._midiNotes[i]._note;
            }
            step._velocity = _props._velocity;
            seq->SetAllStepsPermanent(step);
        }
    }
    else {
        printf("SetAllStepsSeqAction: no seq entity!!\n");
        return;
    }
}

void SetStepSequenceSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _seqEditorId;
    // ASSUMES WE ONLY HAVE ONE LINE
    std::string seqStr;
    std::getline(input, seqStr);
    std::stringstream ss(seqStr);
    StepSequencerEntity::LoadSequenceFromInput(ss, _sequence);
}

void SetStepSequenceSeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "seq_editor_id", _seqEditorId);
    _isSynth = false;
    pt.TryGetBool("is_synth", &_isSynth);
    std::string seqStr = pt.GetString("seq_str");
    std::stringstream ss(seqStr);
    StepSequencerEntity::LoadSequenceFromInput(ss, _sequence);
    _offsetStart = false;
    pt.TryGetBool("offset_start", &_offsetStart);
}

void SetStepSequenceSeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "seq_editor_id", _seqEditorId);
    pt.PutBool("is_synth", _isSynth);
    std::stringstream ss;
    for (StepSequencerEntity::SeqStep const& step : _sequence) {
        StepSequencerEntity::WriteSeqStep(step, _isSynth, ss);
        ss << " ";
    }
    std::string seqStr = ss.str();
    pt.PutString("seq_str", seqStr.c_str());
    pt.PutBool("offset_start", _offsetStart);
}

bool SetStepSequenceSeqAction::ImGui() {
    bool changed = imgui_util::InputEditorId("Seq editor ID", &_seqEditorId);    
    ImGui::Checkbox("Synth", &_isSynth);
    ImGui::Checkbox("Offset start", &_offsetStart);
    if (ImGui::TreeNode("Sequence")) {
        changed = StepSequencerEntity::SeqImGui("Sequence", !_isSynth, _sequence) || changed;
        ImGui::TreePop();
    }

    return changed;
}

void SetStepSequenceSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_seqEditorId, ne::EntityType::StepSequencer, nullptr, "SetStepSequenceSeqAction");
    if (e) {
        _seqId = e->_id;
    }   
}

void SetStepSequenceSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        if (_offsetStart) {
            seq->SetSequencePermanentWithStartOffset(_sequence);
        } else {
            seq->SetSequencePermanent(_sequence);
        }
    }
    else {
        printf("SetStepSequenceSeqAction: no seq entity!!\n");
        return;
    }
}

//void NoteOnOffSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
//    std::string token, key, value;
//    while (!input.eof()) {
//        {
//            input >> token;
//            std::size_t delimIx = token.find_first_of(':');
//            if (delimIx == std::string::npos) {
//                printf("Token missing \":\" - \"%s\"\n", token.c_str());
//                continue;
//            }
//
//            key = token.substr(0, delimIx);
//            value = token.substr(delimIx + 1);
//        }
//        if (key == "channel") {
//            _props._channel = std::stoi(value);
//        }
//        else if (key == "note") {
//            _props._midiNote = GetMidiNote(value.c_str());
//        }
//        else if (key == "length") {
//            _props._noteLength = std::stod(value);
//        }
//        else if (key == "velocity") {
//            _props._velocity = std::stof(value);
//        }
//        else if (key == "quantize") {
//            _props._quantizeDenom = std::stod(value);
//        }
//        else {
//            printf("NoteOnOffSeqAction::Load: unknown key \"%s\"\n", key.c_str());
//        }
//    }
//}

void NoteOnOffSeqAction::LoadDerived(serial::Ptree pt) {
    _props.Load(pt);
    int constexpr kUseIntsForMidiNotesVersion = 3;
    if (pt.GetVersion() < kUseIntsForMidiNotesVersion) {
        if (!_props._midiNotes.empty()) {
            printf("NoteOnOffSeqAction::LoadDerived:  UNEXPECTED NOTES IN THIS VERSION!\n");
            _props._midiNotes.clear();
        }
        std::vector<std::string> noteNames;
        serial::LoadVectorFromChildNode<std::string>(pt, "midiNoteNames", noteNames);
        for (std::string const& noteName : noteNames) {
            int midiNote = GetMidiNote(noteName);
            if (midiNote < 0) {
                continue;
            }            
            _props._midiNotes.emplace_back(midiNote);
        }
    }
}

void NoteOnOffSeqAction::ExecuteDerived(GameManager& g) {
    BeatTimeEvent b_e;    
    b_e._e.channel = _props._channel;    
    b_e._e.velocity = _props._velocity;    

    static int sNoteOnId = 1;
    for (MidiNoteAndName const& midiNote : _props._midiNotes) {
        if (midiNote._note < 0) {
            continue;
        }
        b_e._beatTime = 0.0;
        b_e._e.midiNote = midiNote._note;
        b_e._e.noteOnId = sNoteOnId++;
        b_e._e.type = audio::EventType::NoteOn;

        audio::Event e = GetEventAtBeatOffsetFromNextDenom(_props._quantizeDenom, b_e, *g._beatClock, /*slack=*/0.0625);
        g._audioContext->AddEvent(e);
         
        if (_props._doNoteOff) {
            b_e._e.type = audio::EventType::NoteOff;
            b_e._beatTime = _props._noteLength;
            e = GetEventAtBeatOffsetFromNextDenom(_props._quantizeDenom, b_e, *g._beatClock, /*slack=*/0.0625);
            g._audioContext->AddEvent(e);
        }
    }
}

void NoteOnOffSeqAction::ExecuteRelease(GameManager& g) {
    if (!_props._holdNotes) {
        return;
    }
    audio::Event e;
    e.delaySecs = 0.0;
    e.channel = _props._channel;
    e.type = audio::EventType::NoteOff;
    for (MidiNoteAndName const& midiNote : _props._midiNotes) {
        if (midiNote._note < 0) {
            continue;
        }
        e.midiNote = midiNote._note;        
        g._audioContext->AddEvent(e);
    }
}

void SetStepSequencerMuteSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _seqEditorId;
    input >> _mute;
}

void SetStepSequencerMuteSeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "seq_editor_id", _seqEditorId);
    _mute = pt.GetBool("mute");
    pt.TryGetFloat("gain", &_gain);
}

void SetStepSequencerMuteSeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "seq_editor_id", _seqEditorId);
    pt.PutBool("mute", _mute);
    pt.PutFloat("gain", _gain);
}

bool SetStepSequencerMuteSeqAction::ImGui() {
    imgui_util::InputEditorId("Seq editor ID", &_seqEditorId);
    ImGui::Checkbox("Mute", &_mute);
    ImGui::InputFloat("Gain", &_gain);
    return false;
}

void SetStepSequencerMuteSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_seqEditorId, ne::EntityType::StepSequencer, nullptr, "SetStepSequenceMuteSeqAction");
    if (e) {
        _seqId = e->_id;
    }
}

void SetStepSequencerMuteSeqAction::ExecuteDerived(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        seq->_mute = _mute;
        if (_gain >= 0.f) {
            seq->_gain = _gain;
        }
    }
    else {
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
    if (_quantizeDenom > 0) {
        e = GetEventAtBeatOffsetFromNextDenom(_quantizeDenom, _b_e, *g._beatClock, /*slack=*/0.0625);
    }
    else {
        e = _b_e.ToEvent(*g._beatClock);
    }
    g._audioContext->AddEvent(e);
}

void WaypointControlSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityEditorId;
    input >> _followWaypoints;
}

void WaypointControlSeqAction::LoadDerived(serial::Ptree pt) {
    _followWaypoints = pt.GetBool("follow_waypoints");
    serial::LoadFromChildOf(pt, "entity_editor_id", _entityEditorId);
}

void WaypointControlSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("follow_waypoints", _followWaypoints);
    serial::SaveInNewChildOf(pt, "entity_editor_id", _entityEditorId);
}

bool WaypointControlSeqAction::ImGui() {
    ImGui::Checkbox("Follow", &_followWaypoints);
    imgui_util::InputEditorId("Entity editor ID", &_entityEditorId);
    return false;
}

void WaypointControlSeqAction::ExecuteDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByEditorId(_entityEditorId, nullptr, "WaypointControlSeqAction");
    if (_followWaypoints) {
        e->_wpFollower.Start(g, *e);
    }
    else {
        e->_wpFollower.Stop();
    }
}

void PlayerSetKillZoneSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    _maxZ.reset();
    _killIfBelowCameraView = false;
    _killIfLeftOfCameraView = false;

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
        } else if (key == "kill_below_view") {
            _killIfBelowCameraView = true;
        } else if (key == "kill_left_of_view") {
            _killIfLeftOfCameraView = true;
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

    _killIfBelowCameraView = false;
    pt.TryGetBool("kill_below_view", &_killIfBelowCameraView);

    _killIfLeftOfCameraView = false;
    pt.TryGetBool("kill_left_of_view", &_killIfLeftOfCameraView);
}

void PlayerSetKillZoneSeqAction::SaveDerived(serial::Ptree pt) const {
    if (_maxZ.has_value()) {
        pt.PutFloat("max_z", _maxZ.value());
    }
    pt.PutBool("kill_below_view", _killIfBelowCameraView);
    pt.PutBool("kill_left_of_view", _killIfLeftOfCameraView);
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
    }
    else {
        _maxZ = std::nullopt;
    }

    ImGui::Checkbox("Kill below camera view", &_killIfBelowCameraView);
    ImGui::Checkbox("Kill left of camera view", &_killIfLeftOfCameraView);
    return false;
}

void PlayerSetKillZoneSeqAction::ExecuteDerived(GameManager& g) {
    FlowPlayerEntity* pPlayer = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    pPlayer->_s._killMaxZ = _maxZ;
    pPlayer->_s._killIfBelowCameraView = _killIfBelowCameraView;
    pPlayer->_s._killIfLeftOfCameraView = _killIfLeftOfCameraView;
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
    FlowPlayerEntity* pPlayer = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    assert(pPlayer);
    if (_newSectionId < 0) {
        return;
    }
    int currentSectionId = pPlayer->_s._currentSectionId;
    if (currentSectionId == _newSectionId) {
        return;
    }
    if (currentSectionId >= 0) {
        g._neEntityManager->TagAllSectionEntitiesForDestroy(currentSectionId);
    }
    pPlayer->SetNewSection(g, _newSectionId);
}

void PlayerSetSpawnPointSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    try {
        input >> _spawnPos._x >> _spawnPos._y >> _spawnPos._z;
    }
    catch (std::exception&) {
        printf("PlayerSetSpawnPointSeqAction: could not parse spawn pos.\n");
    }
}

void PlayerSetSpawnPointSeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "spawn_pos", _spawnPos);
    serial::LoadFromChildOf(pt, "action_seq_editor_id", _actionSeqEditorId);
}

void PlayerSetSpawnPointSeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "spawn_pos", _spawnPos);
    serial::SaveInNewChildOf(pt, "action_seq_editor_id", _actionSeqEditorId);
}

bool PlayerSetSpawnPointSeqAction::ImGui() {
    imgui_util::InputVec3("Spawn pos", &_spawnPos);
    imgui_util::InputEditorId("ActionSeq to Activate", &_actionSeqEditorId);
    return false;
}

void PlayerSetSpawnPointSeqAction::InitDerived(GameManager& g) {
    if (_actionSeqEditorId.IsValid()) {
        ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_actionSeqEditorId, ne::EntityType::ActionSequencer, nullptr, "PlayerSetSpawnPointSeqAction");
        if (e) {
            _actionSeq = e->_id;
        } else {
            _actionSeq = ne::EntityId();
        }
    }
}

void PlayerSetSpawnPointSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) { return; }
    FlowPlayerEntity* pPlayer = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    pPlayer->_s._respawnPos = _spawnPos;
    pPlayer->_s._toActivateOnRespawn = _actionSeq;
}

void AddToIntVariableSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    try {
        input >> _varEditorId >> _addAmount;
    }
    catch (std::exception&) {
        printf("AddToIntVariableSeqAction::Load: could not parse input\n");
    }
}

void AddToIntVariableSeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "var_editor_id", _varEditorId);
    _addAmount = pt.GetInt("add_amount");
    _reset = false;
    pt.TryGetBool("reset", &_reset);
}

void AddToIntVariableSeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "var_editor_id", _varEditorId);
    pt.PutInt("add_amount", _addAmount);
    pt.PutBool("reset", _reset);
}

void AddToIntVariableSeqAction::InitDerived(GameManager& g) {
    _entityId = ne::EntityId();
    ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_varEditorId, ne::EntityType::IntVariable, nullptr, "AddToIntVariableSeqAction");
    if (e) {
        _entityId = e->_id;
    }
}

bool AddToIntVariableSeqAction::ImGui() {
    imgui_util::InputEditorId("Var editor id", &_varEditorId);
    ImGui::InputInt("Add amount", &_addAmount);
    ImGui::Checkbox("Reset counter", &_reset);
    return false;
}

void AddToIntVariableSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    IntVariableEntity* e = g._neEntityManager->GetEntityAs<IntVariableEntity>(_entityId);
    if (e) {
        if (_reset) {
            e->Reset();
        } else {
            e->AddToVariable(_addAmount);
        }        
    }
}

void SetEntityActiveSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityEditorId >> _active >> _initOnActivate;
}

void SetEntityActiveSeqAction::LoadDerived(serial::Ptree pt) {
    _entitiesTag = -1;
    pt.TryGetInt("entities_tag", &_entitiesTag);
    serial::LoadFromChildOf(pt, "entity_editor_id", _entityEditorId);
    _active = pt.GetBool("active");
    _initOnActivate = pt.GetBool("init_on_activate");
    _initIfAlreadyActive = false;
    pt.TryGetBool("init_if_already_active", &_initIfAlreadyActive);
}

void SetEntityActiveSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("entities_tag", _entitiesTag);
    serial::SaveInNewChildOf(pt, "entity_editor_id", _entityEditorId);
    pt.PutBool("active", _active);
    pt.PutBool("init_on_activate", _initOnActivate);
    pt.PutBool("init_if_already_active", _initIfAlreadyActive);
}

bool SetEntityActiveSeqAction::ImGui() {
    ImGui::InputInt("Tag", &_entitiesTag);
    imgui_util::InputEditorId("Entity editor ID", &_entityEditorId);
    ImGui::Checkbox("Active", &_active);
    ImGui::Checkbox("Init on activate", &_initOnActivate);
    ImGui::Checkbox("Init if already active", &_initIfAlreadyActive);
    return false;
}

void SetEntityActiveSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = nullptr;
    e = g._neEntityManager->FindEntityByEditorId(_entityEditorId, nullptr, "SetEntityActiveSeqAction");
    if (e) {
        _entityId = e->_id;
    }
}

void SetEntityActiveSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    std::vector<ne::Entity*> entities;
    bool const includeActive = !_active || _initIfAlreadyActive;
    bool const includeInactive = _active;
    if (_entitiesTag >= 0) {
        g._neEntityManager->FindEntitiesByTag(_entitiesTag, includeActive, includeInactive, &entities);
    }
    if (ne::Entity* e = g._neEntityManager->GetActiveOrInactiveEntity(_entityId)) {
        entities.push_back(e);
    }
    if (_active) {
        for (ne::Entity* e : entities) {
            g._neEntityManager->TagForActivate(e->_id, _initOnActivate, _initIfAlreadyActive);
        }
    }
    else {
        for (ne::Entity* e : entities) {
            g._neEntityManager->TagForDeactivate(e->_id);
        }
    }
}

void ChangeStepSeqMaxVoicesSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _props._seqEditorId >> _props._relative >> _props._numVoices;
}

void ChangeStepSeqMaxVoicesSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_props._seqEditorId, ne::EntityType::StepSequencer, nullptr, "ChangeStepSeqMaxVoicesSeqAction");
    if (e) {
        _entityId = e->_id;
    }
}

void ChangeStepSeqMaxVoicesSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    if (StepSequencerEntity* e = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_entityId))) {
        if (_props._relative) {
            e->_maxNumVoices += _props._numVoices;
        }
        else {
            e->_maxNumVoices = _props._numVoices;
        }
    }
}

void TriggerSeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "entity_editor_id", _entityEditorId);
    pt.TryGetBool("do_exit_actions", &_triggerExitActions);
}

void TriggerSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityEditorId;
}

void TriggerSeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "entity_editor_id", _entityEditorId);
    pt.PutBool("do_exit_actions", _triggerExitActions);
}

bool TriggerSeqAction::ImGui() {
    bool changed = imgui_util::InputEditorId("Entity editor ID", &_entityEditorId);
    ImGui::Checkbox("Do exit actions", &_triggerExitActions);
    return changed;
}

void TriggerSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = nullptr;
    e = g._neEntityManager->FindEntityByEditorIdAndType(_entityEditorId, ne::EntityType::FlowTrigger, nullptr, "TriggerSeqAction");
    if (e) {
        _entityId = e->_id;
    }
}

void TriggerSeqAction::ExecuteDerived(GameManager& g) {
    if (ne::Entity* e = g._neEntityManager->GetEntity(_entityId)) {
        FlowTriggerEntity* t = (FlowTriggerEntity*)e;
        if (_triggerExitActions) {
            for (auto const& pAction : t->_p._actionsOnExit) {
                pAction->Execute(g);
            }
        } else {
            t->OnTrigger(g);
        }        
    }
}

void RespawnSeqAction::ExecuteDerived(GameManager& g) {
    if (ne::Entity* e = g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer)) {
        FlowPlayerEntity* player = (FlowPlayerEntity*) e;
        player->RespawnInstant(g);
    }
}

void RandomizeTextSeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "enemy_editor_id", _p.enemy);
}

void RandomizeTextSeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "enemy_editor_id", _p.enemy);
}

bool RandomizeTextSeqAction::ImGui() {
    bool changed = imgui_util::InputEditorId("Enemy editor ID", &_p.enemy);
    return changed;
}

void RandomizeTextSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = nullptr;
    e = g._neEntityManager->FindEntityByEditorIdAndType(_p.enemy, ne::EntityType::TypingEnemy, nullptr, "RandomizeTextSeqAction");
    if (e) {
        _s.enemy = e->_id;
    }
}

void RandomizeTextSeqAction::ExecuteDerived(GameManager& g) {
    if (TypingEnemyEntity* e = g._neEntityManager->GetEntityAs<TypingEnemyEntity>(_s.enemy)) {
        e->RandomizeText();
    }
}

void SetBpmSeqAction::LoadDerived(serial::Ptree pt) {
    pt.TryGetDouble("bpm", &_bpm);
}

void SetBpmSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutDouble("bpm", _bpm);
}

bool SetBpmSeqAction::ImGui() {
    ImGui::InputDouble("bpm", &_bpm);
    return false;
}

void SetBpmSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) { return; }
    g._beatClock->_bpm = _bpm;
}

void SetMissTriggerSeqAction::LoadDerived(serial::Ptree pt) {
    serial::LoadFromChildOf(pt, "trigger", _triggerEditorId);
}

void SetMissTriggerSeqAction::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "trigger", _triggerEditorId);
}

bool SetMissTriggerSeqAction::ImGui() {
    imgui_util::InputEditorId("Trigger", &_triggerEditorId);
    return false;
}

void SetMissTriggerSeqAction::InitDerived(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_triggerEditorId, ne::EntityType::FlowTrigger);
    if (e) {
        _trigger = e->_id;
    } else {
        printf("SetMissTriggerSeqAction::InitDerived: no entity found with editor ID %lld\n", _triggerEditorId._id);
    }
}

void SetMissTriggerSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) { return; }
    FlowPlayerEntity* player = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    player->_s._missTrigger = _trigger;
}