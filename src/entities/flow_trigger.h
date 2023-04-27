#pragma once

#include "new_entity.h"
#include "seq_action.h"

struct FlowTriggerEntity : public ne::Entity {
    // serialized
    std::vector<std::unique_ptr<SeqAction>> _actions;
    
    // non-serialized
    bool _isTriggering = false;
    void OnTrigger(GameManager& g);
       
    virtual void Update(GameManager& g, float dt) override;

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    FlowTriggerEntity() = default;
    FlowTriggerEntity(FlowTriggerEntity const&) = delete;
    FlowTriggerEntity(FlowTriggerEntity&&) = default;
    FlowTriggerEntity& operator=(FlowTriggerEntity&&) = default;
};
