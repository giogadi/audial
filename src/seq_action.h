#pragma once

#include <string>
#include <istream>
#include <optional>

#include "enums/audio_SynthParamType.h"

#include "game_manager.h"
#include "new_entity.h"
#include "beat_time_event.h"
#include "entities/step_sequencer.h"

#include "enums/SeqActionType.h"

#include "properties/SpawnAutomatorSeqAction.h"
#include "properties/ChangeStepSequencerSeqAction.h"
#include "properties/NoteOnOffSeqAction.h"

struct SeqAction;

struct BeatTimeAction {
    BeatTimeAction(BeatTimeAction const& rhs) = delete;
    BeatTimeAction(BeatTimeAction&&) = default;
    BeatTimeAction& operator=(BeatTimeAction&&) = default;
    BeatTimeAction() {}
    std::unique_ptr<SeqAction> _pAction;
    double _beatTime = 0.0;
};

struct SeqAction {

    virtual SeqActionType Type() const = 0;
    
    struct LoadInputs {
        double _beatTimeOffset = 0.0;
        bool _defaultEnemiesSave = false;
        int _sectionId = -1;
    };

    bool _oneTime = false;
    
    void Init(GameManager& g) {
        _hasExecuted = false;
        InitDerived(g);
        _init = true;
    }
    void Execute(GameManager& g) {
        assert(_init);
        if (!_oneTime || !_hasExecuted) {
            ExecuteDerived(g);
            _hasExecuted = true;
        }
    }

    virtual ~SeqAction() {}

    static void LoadAndInitActions(GameManager& g, std::istream& input, std::vector<BeatTimeAction>& actions);
    static std::unique_ptr<SeqAction> LoadAction(LoadInputs const& loadInputs, std::istream& input);

    virtual bool ImGui() { return false; };
    static bool ImGui(char const* label, std::vector<std::unique_ptr<SeqAction>>& actions);

    void Save(serial::Ptree pt) const;
    static void SaveActionsInChildNode(serial::Ptree pt, char const* childName, std::vector<std::unique_ptr<SeqAction>> const& actions);

    static std::unique_ptr<SeqAction> New(SeqActionType actionType);
    static std::unique_ptr<SeqAction> Load(serial::Ptree pt);
    static void LoadActionsFromChildNode(serial::Ptree pt, char const* childName, std::vector<std::unique_ptr<SeqAction>>& actions);
    
protected:
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) = 0;
    virtual void LoadDerived(serial::Ptree pt) {
        printf("UNIMPLEMENTED LOAD - SeqActionType::%s!\n", SeqActionTypeToString(Type()));
    }
    virtual void SaveDerived(serial::Ptree pt) const {
        printf("UNIMPLEMENTED SAVE - SeqActionType::%s!\n", SeqActionTypeToString(Type()));
    }
    virtual void InitDerived(GameManager& g) {}    
    virtual void ExecuteDerived(GameManager& g) = 0;
    
private:
    bool _init = false;
    bool _hasExecuted = false;
};

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
    std::array<int, 4> _midiNotes = {-1, -1, -1, -1};
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
    std::array<int, 4> _midiNotes = {-1, -1, -1, -1};
    
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
    std::vector<StepSequencerEntity::SeqStep> _sequence;
    
    ne::EntityId _seqId;
    
    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) override;
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
