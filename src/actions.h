#pragma once

#include <optional>

#include "seq_action.h"

#include "enums/audio_SynthParamType.h"

#include "entities/step_sequencer.h"

#include "properties/SpawnAutomatorSeqAction.h"
#include "properties/ChangeStepSequencerSeqAction.h"
#include "properties/NoteOnOffSeqAction.h"
#include "properties/ChangeStepSeqMaxVoices.h"

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
    std::string _entityName;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
};

struct ChangeStepSequencerSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::ChangeStepSequencer; }
    ChangeStepSequencerSeqActionProps _props;

    ne::EntityId _seqId;
    std::array<int, 4> _midiNotes = { -1, -1, -1, -1 };
    float _velocity = 0.f;

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
    // serialized
    std::string _seqName;
    std::string _stepStr;
    bool _velOnly = false;

    ne::EntityId _seqId;
    float _velocity = 0.f;
    std::array<int, 4> _midiNotes = { -1, -1, -1, -1 };

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
};

struct SetStepSequenceSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetStepSequence; }
    // serialized
    std::string _seqName;
    std::string _seqStr;

    std::vector<StepSequencerEntity::SeqStep> _sequence;
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
    std::string _seqName;
    bool _mute;

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

    NoteOnOffSeqActionProps _props;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override { _props.Load(pt); }
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
    std::string _entityName;

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

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
};

struct PlayerSetSpawnPointSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::PlayerSetSpawnPoint; }
    Vec3 _spawnPos;
    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
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
    std::string _varName;
    int _addAmount = 0;

    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
};

struct SetEntityActiveSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SetEntityActive; }
    std::string _entityName;
    bool _active = true;
    bool _initOnActivate = true;

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
    virtual void SaveDerived(serial::Ptree pt) const override { _props.Save(pt); }
    virtual bool ImGui() override { return _props.ImGui(); }
    virtual void InitDerived(GameManager& g) override;
    virtual void ExecuteDerived(GameManager& g) override;
};

struct TriggerSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::Trigger; }
    std::string _entityName;

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