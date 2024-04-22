#pragma once

#include "new_entity.h"
#include "seq_action.h"

struct HazardEntity : public ne::BaseEntity {
    virtual ne::EntityType Type() override { return ne::EntityType::Hazard; }
    static ne::EntityType StaticType() { return ne::EntityType::Hazard; }
    
    struct Props {
        std::vector<std::unique_ptr<SeqAction>> destroyActions;
    };
    Props _p;

    struct State {
    };
    State _s;

    // Init() is intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void OnEditPick(GameManager& g) override; 

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override; 
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override; 

    virtual void Draw(GameManager& g, float dt) override;

    HazardEntity() = default;
    HazardEntity(HazardEntity const&) = delete;
    HazardEntity(HazardEntity&&) = default;
    HazardEntity& operator=(HazardEntity&&) = default;

};

