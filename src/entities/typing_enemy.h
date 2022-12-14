#pragma once

#include <vector>

#include "new_entity.h"
#include "seq_action.h"

struct TypingEnemyEntity : public ne::Entity {
    enum class HitBehavior { SingleAction, AllActions };
    
    // serialized
    std::string _text;
    std::vector<std::unique_ptr<SeqAction>> _hitActions;
    HitBehavior _hitBehavior = HitBehavior::SingleAction;
    double _activeBeatTime = -1.0;
    double _inactiveBeatTime = -1.0;
    
    // non-serialized
    int _numHits = 0;
    Vec3 _velocity; // spatial, not audio

    bool IsActive(GameManager& g) const;
    void OnHit(GameManager& g);
    
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
