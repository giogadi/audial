#pragma once

#include "new_entity.h"
#include "enums/MechType.h"

struct MechEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::Mech; }
    static ne::EntityType StaticType() { return ne::EntityType::Mech; }
    
    struct Props {
        MechType type;
        char key = 'a';
    };
    Props _p;

    struct State {
        // For imgui shenanigans
        char keyBuf[2];
    };
    State _s;
       
    virtual void UpdateDerived(GameManager& g, float dt) override;
    virtual void Draw(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;    
};
