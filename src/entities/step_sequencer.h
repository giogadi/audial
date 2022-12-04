#pragma once

#include "new_entity.h"
#include "beat_time_event.h"

struct StepSequencerEntity : ne::Entity {
    // Serialized
    std::vector<int> _initialMidiSequence;
    double _stepBeatLength = 0.25;
    int _channel = 0;
    double _noteLength = 0.25;
    double _loopStartBeatTime = 4.0;
    bool _resetToInitialAfterPlay = true;

    // non-serialized
    int _currentIx = 0;
    std::vector<int> _midiSequence;

    void SetNextSeqStep(int midiNote);  

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) override; */    
};
