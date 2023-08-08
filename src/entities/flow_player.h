#pragma once

#include <deque>
#include <map>
#include <optional>

#include "new_entity.h"

struct FlowPlayerEntity : public ne::Entity {
    // serialized
    float _selectionRadius = -1.f;  // ignored if < 0
    float _launchVel = 4.f; // > 0
    Vec3 _gravity = Vec3(0.f, 0.f, 9.81f);
    float _maxHorizSpeedAfterDash = 20.f;
    float _dashTime = 0.5f;
    Vec3 _respawnPos;
    float _maxSpeed = 20.f; // DUE TO GRAVITY ONLY

    // non-serialized
    Vec3 _vel;
    // TODO: use a ring buffer instead. maybe stack would work too
    std::deque<Vec3> _posHistory;  // start is most recent
    int _framesSinceLastSample = 0;
    int _currentSectionId = -1;
    ne::EntityId _cameraId;
    float _dashTimer = -1.f;
    ne::EntityId _dashTargetId;
    Vec4 _currentColor;
    bool _flowPolarity = true;
    double _countOffEndTime = 3.0;
    std::optional<float> _killMaxZ;  // kill/respawn if player goes over this value
    bool _killIfBelowCameraView = false;
    bool _applyGravityDuringDash = false;
    bool _respawnBeforeFirstDash = true;

    void Draw(GameManager& g);
    void Respawn(GameManager &g);
    void SetNewSection(GameManager& g, int newSectionId);
    
    virtual void InitDerived(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */
};
