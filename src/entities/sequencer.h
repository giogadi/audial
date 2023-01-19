#pragma once

#include "new_entity.h"
#include "beat_time_event.h"

struct SequencerEntity : ne::Entity {
    // Serialized
    bool _loop = false;
    std::vector<BeatTimeEvent> _events;
    double _startBeatTime = 0.0;

    // Gets initialized to false in edit mode.
    bool _playing = true;
    int _currentIx = -1;
    double _currentLoopStartBeatTime = -1.0;    

    void Reset();

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
    
};
