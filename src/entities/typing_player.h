#pragma once

#include <set>
#include <map>

#include "new_entity.h"

struct TypingPlayerEntity : public ne::Entity {
    enum class EnemySelectionType { MinX, NearPos, MinXZ };

    // serialized
    EnemySelectionType _selectionType = EnemySelectionType::MinX;
    float _selectionRadius = 1.f;

    // non-serialized
    ne::EntityId _currentEnemyId;
    std::map<int, std::set<ne::EntityId>> _sectionEnemies;
    std::map<int, int> _sectionNumBeats;
    int _currentSectionId = -2;
    double _nextSectionStartTime = 0.0;
    bool _adjustedForQuant = false;

    void RegisterSectionEnemy(int sectionId, ne::EntityId enemyId);
    void SetSectionLength(int sectionId, int numBeats);
    
    virtual void InitDerived(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */
};
