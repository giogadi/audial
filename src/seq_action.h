#pragma once

#include <string>
#include <istream>

#include "enums/audio_SynthParamType.h"

#include "game_manager.h"
#include "new_entity.h"
#include "beat_time_event.h"
#include "entities/step_sequencer.h"

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
    struct LoadInputs {
        double _beatTimeOffset = 0.0;
        bool _defaultEnemiesSave = false;
        int _sectionId = -1;
    };

    bool _oneTime = false;
    
    void InitBase(GameManager& g) {        
        Init(g);
        _init = true;
    }
    void ExecuteBase(GameManager& g) {
        assert(_init);
        if (!_oneTime || !_hasExecuted) {
            Execute(g);
            _hasExecuted = true;
        }
    }
    void SaveBase(std::ostream& output) {
        if (_oneTime) {
            output << "onetime ";
        }
        Save(output);
    }

    virtual ~SeqAction() {}

    static void LoadAndInitActions(GameManager& g, std::istream& input, std::vector<BeatTimeAction>& actions);
    static std::unique_ptr<SeqAction> LoadAction(LoadInputs const& loadInputs, std::istream& input);
    virtual void Save(std::ostream& output) const { assert(false); }
    
protected:
    virtual void Load(
        LoadInputs const& loadInputs, std::istream& input) = 0;
    virtual void Init(GameManager& g) {}    
    virtual void Execute(GameManager& g) = 0;
    
private:
    bool _init = false;
    bool _hasExecuted = false;
};

struct SpawnAutomatorSeqAction : public SeqAction {
    float _startValue = 0.f;
    float _endValue = 1.f;
    double _desiredAutomateTime = 1.0;
    bool _synth = false;
    audio::SynthParamType _synthParam = audio::SynthParamType::Gain;
    int _channel = 0;  // only if _synth == true
    std::string _seqEntityName;
    
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void Save(std::ostream& output) const override;
};

struct RemoveEntitySeqAction : public SeqAction {
    std::string _entityName;

    virtual void Execute(GameManager& g) override;
    virtual void Load(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void Save(std::ostream& output) const override;
};

struct ChangeStepSequencerSeqAction : public SeqAction {
    // serialized
    std::string _seqName;
    // TODO: Turn these into a StepSequencer::SeqStepChange thing
    std::array<int, 4> _midiNotes = {-1, -1, -1, -1};
    float _velocity = 0.f;
    bool _velOnly = false;
    bool _temporary = true;
    
    ne::EntityId _seqId;    
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        LoadInputs const& loadInputs, std::istream& input) override;    
    virtual void Init(GameManager& g) override;
    virtual void Save(std::ostream& output) const override;
};

// Assumed to be permanent
struct SetAllStepsSeqAction : public SeqAction {
    // serialized
    std::string _seqName;
    std::array<int, 4> _midiNotes = {-1, -1, -1, -1};
    float _velocity = 0.f;

    ne::EntityId _seqId;
    
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void Init(GameManager& g) override;
};

struct SetStepSequenceSeqAction : public SeqAction {
    // serialized
    std::string _seqName;
    std::vector<StepSequencerEntity::SeqStep> _sequence;
    
    ne::EntityId _seqId;
    
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void Init(GameManager& g) override;
};

struct NoteOnOffSeqAction : public SeqAction {
    int _channel = 0;
    int _midiNote = -1;
    double _noteLength = 0.0;  // in beat time
    float _velocity = 1.f;
    double _quantizeDenom = 0.25;
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        LoadInputs const& loadInputs, std::istream& input) override;
};

struct BeatTimeEventSeqAction : public SeqAction {
    // serialize
    std::string _soundBankName;
    BeatTimeEvent _b_e;
    double _quantizeDenom = 0.25;
    
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        LoadInputs const& loadInputs, std::istream& input) override;
    virtual void Init(GameManager& g) override;
};
