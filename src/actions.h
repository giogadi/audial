#pragma once

#include <optional>

#include "seq_action.h"

#include "enums/audio_SynthParamType.h"

#include "entities/step_sequencer.h"

#include "properties/SpawnAutomatorSeqAction.h"
#include "properties/NoteOnOffSeqAction.h"
#include "properties/ChangeStepSeqMaxVoices.h"
#include "properties/SetAllStepsSeqAction.h"

struct SpawnAutomatorSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SpawnAutomator; }
    SpawnAutomatorSeqActionProps _props;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override {
        _props.Load(pt);
    }
    virtual void SaveDerived(serial::Ptree pt) const override {
        _props.Save(pt); 
    }
    virtual bool ImGui() override {
        return _props.ImGui();
    }

};

struct RemoveEntitySeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::RemoveEntity; }
    EditorId _entityEditorId;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
};

struct ChangeStepSequencerSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::ChangeStepSequencer; }
    struct Props {
        EditorId _seqEntityEditorId;

        bool _changeVel;

        bool _changeNote;

        bool _temporary;

        std::vector<MidiNoteAndName> _midiNotes;

        float _velocity;

        std::vector<StepSequencerEntity::SynthParamValue> _params;
    };
    Props _props;

    ne::EntityId _seqId;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void InitDerived(GameManager& g) override;
    virtual bool ImGui() override;
};

// Assumed to be permanent
struct SetAllStepsSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetAllSteps; }
    SetAllStepsSeqActionProps _props;

    ne::EntityId _seqId;

    virtual void ExecuteDerived(GameManager& g) override;
    // virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
};

struct SetStepSequenceSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetStepSequence; }
    // serialized
    EditorId _seqEditorId;
    std::vector<StepSequencerEntity::SeqStep> _sequence;
    bool _offsetStart = false;
    bool _isSynth = false;

    ne::EntityId _seqId;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
};

struct SetStepSequencerMuteSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetStepSequencerMute; }
    // serialized
    EditorId _seqEditorId;
    bool _mute;
    float _gain = -1.f;

    ne::EntityId _seqId;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
};

struct NoteOnOffSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::NoteOnOff; }
    virtual void ExecuteRelease(GameManager& g) override;

    NoteOnOffSeqActionProps _props;

    virtual void ExecuteDerived(GameManager& g) override;
    /*virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;*/
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override { _props.Save(pt); }
    virtual bool ImGui() override { return _props.ImGui(); }
};

struct BeatTimeEventSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::BeatTimeEvent; }
    // serialize
    BeatTimeEvent _b_e;
    double _quantizeDenom = 0.25;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
};

struct WaypointControlSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::WaypointControl; }
    // serialize
    bool _followWaypoints = true;
    EditorId _entityEditorId;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
};

struct PlayerSetKillZoneSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::PlayerSetKillZone; }
    std::optional<float> _maxZ;
    bool _killIfBelowCameraView = false;
    bool _killIfLeftOfCameraView = false;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
};

struct PlayerSetSpawnPointSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::PlayerSetSpawnPoint; }
    
    Vec3 _spawnPos;
    EditorId _positionEditorId;
    EditorId _actionSeqEditorId;

    ne::EntityId _actionSeq;
    ne::EntityId _posEntity;

    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct SetNewFlowSectionSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetNewFlowSection; }
    int _newSectionId = -1;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
};

struct AddToIntVariableSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::AddToIntVariable; }
    EditorId _varEditorId;
    int _addAmount = 0;
    bool _reset = false; // if true, _addAmount is ignored

    ne::EntityId _entityId;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void InitDerived(GameManager& g) override;
    virtual bool ImGui() override;
};

struct SetEntityActiveSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetEntityActive; }
    EditorId _entityEditorId;
    int _entitiesTag = -1;
    bool _active = true;
    bool _initOnActivate = true;
    bool _initIfAlreadyActive = false;

    ne::EntityId _entityId;

    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;

    virtual void InitDerived(GameManager& g) override;

    virtual void ExecuteDerived(GameManager& g) override;
};

struct ChangeStepSeqMaxVoicesSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::ChangeStepSeqMaxVoices; }
    ChangeStepSeqMaxVoicesProps _props;

    ne::EntityId _entityId;

    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override { _props.Load(pt); }
    virtual void SaveDerived(serial::Ptree pt) const override {
        _props.Save(pt);        
    }
    virtual bool ImGui() override { return _props.ImGui(); }
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct TriggerSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::Trigger; }
    EditorId _entityEditorId;
    bool _triggerExitActions = false;

    ne::EntityId _entityId;

    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct RespawnSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::Respawn; }

    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override {}
    virtual void LoadDerived(serial::Ptree pt) override {}
    virtual void SaveDerived(serial::Ptree pt) const override {}
    virtual bool ImGui() override { return false; }
    virtual void InitDerived(GameManager& g) override {}
    virtual void ExecuteDerived(GameManager& g) override;
};

struct RandomizeTextSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::RandomizeText; }

    struct Props {
        EditorId enemy;
    };
    Props _p;

    struct State {
        ne::EntityId enemy;
    };
    State _s;

    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct ChangeTextSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::ChangeText; }

    struct Props {
        EditorId enemy;
        std::string text;
    };
    Props _p;

    struct State {
        ne::EntityId enemy;
    };
    State _s;

    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct SetBpmSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetBpm; }
    double _bpm;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct SetMissTriggerSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetMissTrigger; }
    EditorId _triggerEditorId;
    ne::EntityId _trigger;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct SetPlayerResetTriggerSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetPlayerResetTrigger; }
    EditorId _triggerEditorId;
    ne::EntityId _trigger;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct RandomizePositionSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::RandomizePosition; }
    EditorId _editorId;
    EditorId _regionEditorId;
    bool _randomizeY = false;
    

    ne::EntityId _id;
    ne::EntityId _regionId;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};