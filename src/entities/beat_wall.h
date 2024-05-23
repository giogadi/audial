#pragma once

#include "new_entity.h"
#include "enums/Direction.h"

struct BeatWallEntity : public ne::BaseEntity {
    virtual ne::EntityType Type() override { return ne::EntityType::BeatWall; }
    static ne::EntityType StaticType() { return ne::EntityType::BeatWall; }
    
    struct Props {
        int numSteps = 4;
        float spacing = 0.f;
        double beatOffset = -0.5;
        Direction direction = Direction::Right;

    };
    Props _p;

    struct State {
        std::vector<ne::EntityId> walls;
    };
    State _s;

    // Init() is intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void OnEditPick(GameManager& g) override; 

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override; 
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
    virtual ImGuiResult MultiImGui(GameManager& g, BaseEntity** entities, size_t entityCount) override;

    virtual void UpdateDerived(GameManager& g, float dt) override; 

    virtual void Draw(GameManager& g, float dt) override;

    BeatWallEntity() = default;
    BeatWallEntity(BeatWallEntity const&) = delete;
    BeatWallEntity(BeatWallEntity&&) = default;
    BeatWallEntity& operator=(BeatWallEntity&&) = default;

};

