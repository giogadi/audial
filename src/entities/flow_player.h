#pragma once

#include <deque>
#include <map>
#include <optional>

#include "new_entity.h"
#include "enums/TypingEnemyType.h"
#include "input_manager.h"
#include "ring_buffer.h"

struct SeqAction;

struct FlowPlayerEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::FlowPlayer; }
    static ne::EntityType StaticType() { return ne::EntityType::FlowPlayer; }
    
    struct Props {
        float _defaultLaunchVel = 4.f; // > 0
        Vec3 _gravity = Vec3(0.f, 0.f, 9.81f);
        float _maxHorizSpeedAfterDash = 2.f;
        float _maxVertSpeedAfterDash = 0.f;
        float _maxOverallSpeedAfterDash = -1.f;
        float _dashTime = 0.5f;
        float _maxFallSpeed = 10.f;
        bool _pullManualHold = false;
        float _pullStopTime = 0.5f;
        EditorId _deathStartTriggerEditorId;
        EditorId _deathEndTriggerEditorId;
    };
    Props _p;

    enum class MoveState {
        Default, WallBounce, Pull, PullStop, Push, Carried
    };
    enum class DashAnimState {
        None, Accel, Decel
    };
    struct HeldAction {
        InputManager::Key _key = InputManager::Key::NumKeys;
        std::vector<std::unique_ptr<SeqAction>> _actions;
    };
    struct TailState {
            Vec3 p;
            Vec3 v;
    };
    struct Miss {
        char c;
        double t;
    };
    struct State {
        State(State const& rhs) = delete;
        State(State&&) = default;
        State& operator=(State&&) = default;
        State() {}

        Vec3 _vel;
        
        MoveState _moveState = MoveState::Default;

        ne::EntityId _lastHitEnemy;
        ne::EntityId _interactingEnemyId;
        ne::EntityId _carrierId;
        bool _stopAfterTimer = true;
        float _dashTimer = -1.f; // wallBounce/pull/push
        float _currentDashEndTime = -1.f; // pull/push
        float _currentPullStopEndTime = -1.f; // pull/push
        Vec3 _lastKnownDashTarget; // pull/push
        Vec4 _lastKnownDashTargetColor; // pull/push
        bool _stopDashOnPassEnemy = true; // pull
        bool _passedPullTarget = false; // pull
        InputManager::Key _heldKey = InputManager::Key::NumKeys;
        std::vector<HeldAction> _heldActions;        
        bool _haveSeenKeyUpSinceLastHit[static_cast<size_t>(InputManager::Key::NumKeys)];
        int _keyBufferTimeLeft[static_cast<size_t>(InputManager::Key::NumKeys)];

        // anim states
        DashAnimState _dashAnimState = DashAnimState::None;
        float _dashSquishFactor = 0.f;
        Vec3 _dashAnimDir;
        float _dashLaunchSpeed = 0.f;
                
        std::vector<TailState> _tail;
        int _currentSectionId = -1;
        ne::EntityId _cameraId;
        Vec4 _currentColor;
        double _countOffEndTime = 4.0;
        std::optional<float> _killMaxZ;  // kill/respawn if player goes over this value
        bool _killIfBelowCameraView = false;
        bool _killIfLeftOfCameraView = false;
        bool _respawnBeforeFirstInteract = true;
        ne::EntityId _toActivateOnRespawn;
        Vec3 _respawnPos;

        int _respawnLoopLength = 1;
        double _respawnStartBeatTime = -1.0;
        bool _respawnHasResetScene = false;

        ne::EntityId _deathStartTrigger;
        ne::EntityId _deathEndTrigger;
        ne::EntityId _resetTrigger;
        ne::EntityId _missTrigger;

        RingBuffer<Miss, 8> _misses;
    };
    State _s;    

    void RespawnInstant(GameManager &g);
    void StartRespawn(GameManager& g);
    void SetNewSection(GameManager& g, int newSectionId);
    
    virtual void InitDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override;
    virtual void Draw(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};
