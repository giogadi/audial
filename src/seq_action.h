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
    virtual void Execute(GameManager& g) = 0;
    struct LoadInputs {
        double _beatTimeOffset = 0.0;
        bool _defaultEnemiesSave = false;
        int _sectionId = -1;
    };

    virtual ~SeqAction() {}

    static void LoadActions(GameManager& g, std::istream& input, std::vector<BeatTimeAction>& actions);
    static std::unique_ptr<SeqAction> LoadAction(GameManager& g, LoadInputs const& loadInputs, std::istream& input);

protected:
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) {}
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
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

struct RemoveEntitySeqAction : public SeqAction {
    std::string _entityName;

    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

struct ChangeStepSequencerSeqAction : public SeqAction {

    ne::EntityId _seqId;
    std::array<int, 4> _midiNotes = {-1, -1, -1, -1};
    float _velocity = 0.f;
    bool _velOnly = false;
    bool _temporary = true;
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

// Assumed to be permanent
struct SetAllStepsSeqAction : public SeqAction {
    ne::EntityId _seqId;
    std::array<int, 4> _midiNotes = {-1, -1, -1, -1};
    float _velocity = 0.f;
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

struct SetStepSequenceSeqAction : public SeqAction {
    ne::EntityId _seqId;
    std::vector<StepSequencerEntity::SeqStep> _sequence;
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

struct NoteOnOffSeqAction : public SeqAction {
    int _channel = 0;
    int _midiNote = -1;
    double _noteLength = 0.0;  // in beat time
    float _velocity = 1.f;
    double _quantizeDenom = 0.25;
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

struct BeatTimeEventSeqAction : public SeqAction {
    BeatTimeEvent _b_e;
    double _quantizeDenom = 0.25;
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};
