#pragma once

#include "new_entity.h"

struct TestEntity : public ne::BaseEntity {
    virtual ne::EntityType Type() override { return ne::EntityType::Test; }
    static ne::EntityType StaticType() { return ne::EntityType::Test; }
    
    struct Props {
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
    virtual ImGuiResult MultiImGui(GameManager& g, BaseEntity** entities, size_t entityCount) override;

    virtual void UpdateDerived(GameManager& g, float dt) override; 

    virtual void Draw(GameManager& g, float dt) override;

    TestEntity() = default;
    TestEntity(TestEntity const&) = delete;
    TestEntity(TestEntity&&) = default;
    TestEntity& operator=(TestEntity&&) = default;

};

