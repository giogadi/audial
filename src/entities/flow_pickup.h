#pragma once

#include "new_entity.h"

struct SeqAction;

struct FlowPickupEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::FlowPickup; }
    static ne::EntityType StaticType() { return ne::EntityType::FlowPickup; }
    
    // serialized
    std::vector<std::unique_ptr<SeqAction>> _actions;
    bool _killOnPickup;
    Vec4 _hitColor;

    // non-serialized
    bool _hit = false; 
    double _beatTimeOfDeath = -1.0;

    void OnHit(GameManager& g);
       
    virtual void UpdateDerived(GameManager& g, float dt) override;
    virtual void Draw(GameManager& g, float dt) override;

    /* virtual void Destroy(GameManager& g) {} */
    virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    FlowPickupEntity() = default;
    FlowPickupEntity(FlowPickupEntity const&) = delete;
    FlowPickupEntity(FlowPickupEntity&&) = default;
    FlowPickupEntity& operator=(FlowPickupEntity&&) = default;
};
