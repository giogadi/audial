#pragma once

#include "new_entity.h"
#include "mech_button.h"
#include "seq_action.h"

struct SpawnerEntity : public ne::BaseEntity {
    virtual ne::EntityType Type() override { return ne::EntityType::Spawner; }
    static ne::EntityType StaticType() { return ne::EntityType::Spawner; }
    
    struct Props {
        bool hasButton = true;
        double quantize = 0.0;
        std::vector<std::unique_ptr<SeqAction>> actions;
        bool autoSpawn = false;
        Vec3 spawnStep;
    };
    Props _p;

    struct State {
        double actionBeatTime = -1.0;
    };
    State _s;


    MechButton _btn;

    // Init() is intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void OnEditPick(GameManager& g) override; 

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override; 
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override; 

    virtual void Draw(GameManager& g, float dt) override;

    SpawnerEntity() = default;
    SpawnerEntity(SpawnerEntity const&) = delete;
    SpawnerEntity(SpawnerEntity&&) = default;
    SpawnerEntity& operator=(SpawnerEntity&&) = default;

};

