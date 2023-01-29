#pragma once

#include <istream>

#include "new_entity.h"
#include "seq_action.h"

struct ActionSequencerEntity : ne::Entity {    
    ActionSequencerEntity() {}
    ActionSequencerEntity(ActionSequencerEntity&&) = default;
    ActionSequencerEntity& operator=(ActionSequencerEntity&&) = default;
    ActionSequencerEntity(ActionSequencerEntity const& rhs) = delete;

    // serialized
    std::string _seqFilename;

    // non-serialized
    std::vector<BeatTimeAction> _actions;
    int _currentIx = 0;
    // double _currentLoopStartBeatTime = -1.0;

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) override; */    
};
