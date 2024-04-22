#pragma once

#include "new_entity.h"
#include "seq_action.h"
#include "imgui_str.h"
#include "mech_button.h"

struct SinkEntity : public ne::BaseEntity {
    virtual ne::EntityType Type() override { return ne::EntityType::Sink; }
    static ne::EntityType StaticType() { return ne::EntityType::Sink; }
    
    struct Props {
        double quantize = 0.0;
        std::vector<std::unique_ptr<SeqAction>> actions;
        std::vector<std::unique_ptr<SeqAction>> acceptActions;
    };
    Props _p;

    struct State {
        double actionBeatTime = -1.0;
        float acceptTimeElapsed = -1.f;
    };
    State _s;

    MechButton _key;

    // Init() is intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void OnEditPick(GameManager& g) override; 

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override; 
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override; 

    virtual void Draw(GameManager& g, float dt) override;

    SinkEntity() = default;
    SinkEntity(SinkEntity const&) = delete;
    SinkEntity(SinkEntity&&) = default;
    SinkEntity& operator=(SinkEntity&&) = default;

};

