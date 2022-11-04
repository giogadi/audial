#pragma once

#include "new_entity.h"

struct PlayerEntity : public ne::Entity {
    virtual void Init(GameManager& g);
    virtual void Update(GameManager& g, float dt);
    // virtual void Destroy(GameManager& g) {}
    // virtual void OnEditPick(GameManager& g) {}
    // virtual void DebugPrint();

    void ScriptUpdate(GameManager& g);

protected:
    virtual void SaveDerived(serial::Ptree pt) const {};
    virtual void LoadDerived(serial::Ptree pt) {};
    virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; }
};