#pragma once

#include "new_entity.h"

struct TypingPlayerEntity : public ne::Entity {
    // serialized
    std::string _spawnFilename;
    
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */
};
