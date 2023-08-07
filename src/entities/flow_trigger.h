#pragma once

#include "new_entity.h"
#include "seq_action.h"

struct FlowTriggerEntity : public ne::Entity {
    // serialized
    struct Props {
        std::vector<std::unique_ptr<SeqAction>> _actions;
        std::vector<std::unique_ptr<SeqAction>> _actionsOnExit;
        std::vector<std::string> _triggerVolumeEntityNames;
        bool _triggerOnPlayerEnter = true;
        int _randomActionCount = -1;
        bool _useTriggerVolumes = false;
    };
    Props _p;
    
    // non-serialized
    bool _isTriggering = false;
    std::vector<int> _randomDrawList;
    std::vector<ne::EntityId> _triggerVolumeEntities;

    
    void OnTrigger(GameManager& g);
       
    virtual void UpdateDerived(GameManager& g, float dt) override;

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    FlowTriggerEntity() = default;
    FlowTriggerEntity(FlowTriggerEntity const&) = delete;
    FlowTriggerEntity(FlowTriggerEntity&&) = default;
    FlowTriggerEntity& operator=(FlowTriggerEntity&&) = default;
};
