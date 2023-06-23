#pragma once

#include "new_entity.h"

#include "properties/VfxEntity.h"

struct VfxEntity : public ne::Entity {

    VfxEntityProps _p;

    // int _phase = -1;
    float _timeSinceStart = -1.f;
    
    void OnHit(GameManager& g);
       
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    // virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override { _p.Save(pt); }
    virtual void LoadDerived(serial::Ptree pt) override { _p.Load(pt); }
    virtual ImGuiResult ImGuiDerived(GameManager& g) override {
        bool needsInit = _p.ImGui();
        return needsInit ? ImGuiResult::Done : ImGuiResult::NeedsInit;
    }

    VfxEntity() = default;
    VfxEntity(VfxEntity const&) = delete;
    VfxEntity(VfxEntity&&) = default;
    VfxEntity& operator=(VfxEntity&&) = default;
};
