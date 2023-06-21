#pragma once

#include "new_entity.h"

struct SeqAction;

struct FlowPickupEntity : public ne::Entity {
    // serialized
    std::vector<std::unique_ptr<SeqAction>> _actions;

    // non-serialized
    bool _hit = false;

    void OnHit(GameManager& g);
       
    virtual void Update(GameManager& g, float dt) override;
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
