#pragma once

#include "new_entity.h"

struct TypingEnemyEntity : public ne::Entity {
    // serialized
    std::string _text;
    
    // non-serialized
    int _numHits = 0;
    
    /* virtual void Init(GameManager& g); */
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    /* virtual void SaveDerived(serial::Ptree pt) const {}; */
    /* virtual void LoadDerived(serial::Ptree pt) {}; */
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */
};
