#pragma once

#include <deque>
#include <map>

#include "new_entity.h"

struct FlowPlayerEntity : public ne::Entity {
    // serialized
    float _selectionRadius = -1.f;  // ignored if < 0
    float _launchVel = 4.f; // > 0
    Vec3 _gravity = Vec3(0.f, 0.f, 9.81f);
    float _maxHorizSpeedAfterDash = 20.f;
    float _dashTime = 0.5f;

    // non-serialized
    Vec3 _vel;
    // TODO: use a ring buffer instead. maybe stack would work too
    std::deque<Vec3> _posHistory;  // start is most recent
    int _framesSinceLastSample = 0;
    int _currentSectionId = -1;
    enum SectionDir { Center, Up, Down, Left, Right };
    std::map<int, SectionDir> _sectionDirs;
    ne::EntityId _cameraId;
    float _dashTimer = -1.f;
    
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */
};
