#pragma once

#include <string>
#include <istream>

#include "enums/audio_SynthParamType.h"

#include "game_manager.h"
#include "new_entity.h"
#include "beat_time_event.h"

struct SeqAction {
    enum class Type {
        SpawnAutomator,
        RemoveEntity,
        ChangeStepSequencer,
        NoteOnOff,
        BeatTimeEvent,
        SpawnEnemy,
    };
    virtual Type GetType() const = 0;
    virtual void Execute(GameManager& g) = 0;
    struct LoadInputs {
        double _beatTimeOffset = 0.0;
    };
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) {}

    virtual ~SeqAction() {}
};

struct SpawnAutomatorSeqAction : public SeqAction {
    float _startValue = 0.f;
    float _endValue = 1.f;
    double _desiredAutomateTime = 1.0;
    bool _synth = false;
    audio::SynthParamType _synthParam = audio::SynthParamType::Gain;
    int _channel = 0;  // only if _synth == true
    std::string _seqEntityName;
    
    virtual Type GetType() const override { return Type::SpawnAutomator; }
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

struct RemoveEntitySeqAction : public SeqAction {
    std::string _entityName;

    virtual Type GetType() const override { return Type::RemoveEntity; }
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

struct ChangeStepSequencerSeqAction : public SeqAction {
    ne::EntityId _seqId;
    int _midiNote = -1;
    float _velocity = 0.f;
    virtual Type GetType() const override { return Type::ChangeStepSequencer; }
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
    virtual Type GetType() const override { return Type::NoteOnOff; }
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};

struct BeatTimeEventSeqAction : public SeqAction {
    BeatTimeEvent _b_e;
    double _quantizeDenom = 0.25;
    virtual Type GetType() const override { return Type::BeatTimeEvent; }
    virtual void Execute(GameManager& g) override;
    virtual void Load(
        GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};