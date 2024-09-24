#pragma once

#include <sstream>

#include "new_entity.h"
#include "seq_action.h"

struct ActionSequencerEntity : ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::ActionSequencer; }
    static ne::EntityType StaticType() { return ne::EntityType::ActionSequencer; }
    
    ActionSequencerEntity() {}
    ActionSequencerEntity(ActionSequencerEntity&&) = default;
    ActionSequencerEntity& operator=(ActionSequencerEntity&&) = default;
    ActionSequencerEntity(ActionSequencerEntity const& rhs) = delete;

    // serialized
    std::string _seqFilename;
    std::vector<BeatTimeAction> _actions;
    bool _useFile = true;

    // non-serialized
    std::stringstream _actionsString;  // populated at Load() but not fully serialized
    int _currentIx = 0;
    double _startTime = -1.0;

    virtual void InitDerived(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};
