#pragma once

#include "new_entity.h"

struct ResourceEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::Resource; }
    static ne::EntityType StaticType() { return ne::EntityType::Resource; }
    
    struct Props {
    };
    Props _p;

    struct State {
        bool destroyed = false;
        Vec3 v;
        float expandTimeElapsed = -1.f;
        float destroyTimeElapsed = -1.f;
        Transform renderT;
    };
    State _s;

    void StartConsume();
    void StartDestroy(GameManager& g);
       
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
