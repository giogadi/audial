#pragma once

#include <vector>

#include "new_entity.h"
#include "seq_action.h"
#include "input_manager.h"

struct FlowEnemyEntity : public ne::Entity {    
    // serialized
    std::string _text;
    std::vector<std::unique_ptr<SeqAction>> _hitActions;
    HitBehavior _hitBehavior = HitBehavior::SingleAction;
    
    // non-serialized
    Vec3 _velocity; // spatial, not audio
    Vec4 _currentColor;

    bool IsActive(GameManager& g) const;
    void OnHit(GameManager& g);
    InputManager::Key GetNextKey() const;
    
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    /* virtual void SaveDerived(serial::Ptree pt) const {}; */
    /* virtual void LoadDerived(serial::Ptree pt) {}; */
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */

    TypingEnemyEntity() = default;
    TypingEnemyEntity(TypingEnemyEntity const&) = delete;
    TypingEnemyEntity(TypingEnemyEntity&&) = default;
    TypingEnemyEntity& operator=(TypingEnemyEntity&&) = default;
};
