#pragma once

#include <queue>
#include <optional>

#include "new_entity.h"
#include "beat_time_event.h"

struct StepSequencerEntity : ne::Entity {
    struct SeqStep {
        SeqStep() {}
        std::array<int,4> _midiNote = {-1, -1, -1, -1};
        float _velocity = 1.f;
    };
    struct SeqStepChange {
        SeqStep _step;
        bool _changeNote = true;
        bool _changeVelocity = true;
        bool _temporary = true;
    };
    // Serialized
    std::vector<SeqStep> _initialMidiSequenceDoNotChange;
    double _stepBeatLength = 0.25;
    std::vector<int> _channels;
    bool _isSynth = true;  // false is drumkit
    double _noteLength = 0.25;
    double _initialLoopStartBeatTime = 4.0;
    bool _startMute = false;
    bool _editorMute = false;
    int _initMaxNumVoices = -1;

    // non-serialized
    bool _mute = false;
    int _currentIx = 0;
    double _loopStartBeatTime = 0.0;    
    std::vector<SeqStep> _permanentSequence;
    std::vector<SeqStep> _tempSequence;
    std::queue<SeqStepChange> _changeQueue;
    int _maxNumVoices = -1;

    enum StepSaveType { Temporary, Permanent };
    void SetNextSeqStep(GameManager& g, SeqStep step, StepSaveType saveType);
    void SetNextSeqStepVelocity(GameManager& g, float v, StepSaveType saveType);
    void SetAllVelocitiesPermanent(float newVelocity);
    void SetAllStepsPermanent(SeqStep const& newStep);
    void SetSequencePermanent(std::vector<SeqStep> newSequence);

    virtual void InitDerived(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    static void WriteSeqStep(SeqStep const& step, std::ostream& output);
    static bool TryReadSeqStep(std::istream& input, SeqStep& step);

    // Assumes input contains _only_ the sequence, nothing else.
    static void LoadSequenceFromInput(
        std::istream& input, std::vector<SeqStep>& sequence);    
};
