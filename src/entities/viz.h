#pragma once

#include "new_entity.h"

struct VizEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::Viz; }
    static ne::EntityType StaticType() { return ne::EntityType::Viz; }
    
    struct Props {
    };
    Props _p;

    struct State {
        Vec3 v;
    };
    State _s;
       
    virtual void UpdateDerived(GameManager& g, float dt) override;
    virtual void Draw(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    // virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;    
};
