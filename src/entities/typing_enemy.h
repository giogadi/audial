#pragma once

#include <vector>

#include "new_entity.h"
#include "seq_action.h"
#include "input_manager.h"

struct TypingEnemyEntity : public ne::Entity {
    enum class HitBehavior { SingleAction, AllActions };
    
    // serialized
    std::string _text;
    std::vector<std::unique_ptr<SeqAction>> _hitActions;
    HitBehavior _hitBehavior = HitBehavior::SingleAction;
    double _activeBeatTime = -1.0;
    double _inactiveBeatTime = -1.0;
    bool _destroyIfOffscreenLeft = true;
    bool _destroyAfterTyped = true;
    int _sectionId = -1;
    Vec3 _sectionLocalPos;
    int _flowSectionId = -1;
    Vec4 _textColor = Vec4(1.f, 1.f, 1.f, 1.f);
    
    // non-serialized
    int _numHits = 0;
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

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    TypingEnemyEntity() = default;
    TypingEnemyEntity(TypingEnemyEntity const&) = delete;
    TypingEnemyEntity(TypingEnemyEntity&&) = default;
    TypingEnemyEntity& operator=(TypingEnemyEntity&&) = default;
};
