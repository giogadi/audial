#pragma once

#include <queue>

#include "new_entity.h"
#include "beat_time_event.h"

struct StepSequencerEntity : ne::Entity {
    struct SeqStep {
        SeqStep() {}
        SeqStep(int m, float v) : _midiNote(m), _velocity(v) {}
        int _midiNote = -1;
        float _velocity = 1.f;
    };
    // Serialized
    std::vector<SeqStep> _initialMidiSequenceDoNotChange;
    double _stepBeatLength = 0.25;
    std::vector<int> _channels;
    double _noteLength = 0.25;
    double _initialLoopStartBeatTime = 4.0;
    bool _resetToInitialAfterPlay = true;

    // non-serialized
    int _currentIx = 0;
    double _loopStartBeatTime = 0.0;
    std::vector<SeqStep> _initialMidiSequence;
    std::vector<SeqStep> _midiSequence;
    std::queue<SeqStep> _changeQueue;

    void SetNextSeqStep(GameManager& g, int midiNote, float velocity);

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) override; */    
};
