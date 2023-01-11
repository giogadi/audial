#pragma once

#include "new_entity.h"

struct TypingPlayerEntity : public ne::Entity {
    enum class EnemySelectionType { MinX, NearPos };

    // serialized
    EnemySelectionType _selectionType = EnemySelectionType::MinX;
    float _selectionRadius = 1.f;

    // non-serialized
    ne::EntityId _currentEnemyId;
    
    // virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */
};
