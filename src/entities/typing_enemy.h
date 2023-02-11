#pragma once

#include <vector>

#include "new_entity.h"
#include "seq_action.h"
#include "input_manager.h"

struct TypingEnemyEntity : public ne::Entity {
    enum class HitBehavior { SingleAction, AllActions };
    
    // serialized
    std::string _text;
    std::vector<std::string> _hitActionStrings;
    std::vector<bool> _isOneTimeAction;
    HitBehavior _hitBehavior = HitBehavior::SingleAction; // TODO
    double _activeBeatTime = -1.0; // TODO
    double _inactiveBeatTime = -1.0; // TODO
    bool _destroyIfOffscreenLeft = true; // TODO
    bool _destroyAfterTyped = true;
    int _sectionId = -1; // TODO
    Vec3 _sectionLocalPos; // TODO
    int _flowSectionId = -1;
    Vec4 _textColor = Vec4(1.f, 1.f, 1.f, 1.f); // TODO
    bool _flowPolarity = false;
    Vec3 _prevWaypointPos;
    struct Waypoint {
        float _t;  // time in seconds to get to this point from the previous one.
        Vec3 _p;
        void Save(serial::Ptree pt) const;
        void Load(serial::Ptree pt);
    };
    std::vector<Waypoint> _waypoints;
    
    // non-serialized
    int _numHits = 0;
    Vec3 _velocity; // spatial, not audio
    Vec4 _currentColor;
    std::vector<std::unique_ptr<SeqAction>> _hitActions;
    bool _useHitActionsOnInitHack = false;
    float _currentWaypointTime = 0.f;
    bool _followingWaypoints = true;
    int _currentWaypointIx = 0;

    bool IsActive(GameManager& g) const;
    void OnHit(GameManager& g);
    void DoHitActions(GameManager& g);
    InputManager::Key GetNextKey() const;

    
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    TypingEnemyEntity() = default;
    TypingEnemyEntity(TypingEnemyEntity const&) = delete;
    TypingEnemyEntity(TypingEnemyEntity&&) = default;
    TypingEnemyEntity& operator=(TypingEnemyEntity&&) = default;

    static void MultiSelectImGui(GameManager& g, std::vector<TypingEnemyEntity*>& enemies);
};
