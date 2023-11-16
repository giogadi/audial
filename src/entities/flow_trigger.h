#pragma once

#include "new_entity.h"
#include "seq_action.h"

struct FlowTriggerEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::FlowTrigger; }
    static ne::EntityType StaticType() { return ne::EntityType::FlowTrigger; }
    
    // serialized
    struct Props {
        double _triggerDelayBeatTime = -1.0;
        std::vector<std::unique_ptr<SeqAction>> _actions;
        std::vector<std::unique_ptr<SeqAction>> _actionsOnExit;
        std::vector<EditorId> _triggerVolumeEditorIds;
        bool _triggerOnPlayerEnter = true;
        int _randomActionCount = -1;
        bool _useTriggerVolumes = false;
        bool _executeOnInit = false;
    };
    Props _p;
    
    // non-serialized
    bool _isTriggering = false;
    std::vector<int> _randomDrawList;
    std::vector<ne::EntityId> _triggerVolumeEntities;
    double _triggerTime = -1.0;  // time to trigger, AFTER APPLYING DELAY.
    bool _didInitTrigger = false;
    
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

private:
    void RunActions(GameManager& g);
};
