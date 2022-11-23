#pragma once

#include "new_entity.h"
#include "input_manager.h"
#include "constants.h"
#include "beat_time_event.h"

struct LaneNoteEnemyEntity : public ne::Entity {
    enum class Behavior {
        None,
        ConstVel,
        Zigging,
        MoveOnPhase,
    };
    enum class OnHitBehavior {
        Default, // decrement hp, doot, die at 0 hp
        MultiPhase, // decrement hp, doot, increment phase at 0 hp, phase doot
    };
    enum class LaneNoteBehavior {
        Table, // use lane notes tables
        Events // use _events
    };
    static int constexpr kMaxNumNotesPerLane = 4;
    struct LaneNotesTable {
        double _noteLength = 0.25;
        int _channel = 0;
        std::array<std::array<int, kMaxNumNotesPerLane>, kNumLanes> _table;
    };

    // serialized
    double _eventStartDenom = 0.125;
    InputManager::Key _shootButton = InputManager::Key::NumKeys;
    double _activeBeatTime = 0.0;
    double _inactiveBeatTime = -1.0;
    Behavior _behavior = Behavior::None;
    OnHitBehavior _onHitBehavior = OnHitBehavior::Default;
    LaneNoteBehavior _laneNoteBehavior = LaneNoteBehavior::Table;
    int _initialHp = -1;
    // ConstVel-specific
    float _constVelX = 0.f;
    float _constVelZ = 0.f;
    // MultiPhase-specific
    int _numHpBars = 1;

    float _desiredSpawnY = 0.f;
    double _shotBeatTime = -1.0;
    int _hp = -1;

    Vec3 _motionSource;
    Vec3 _motionTarget;
    double _motionStartBeatTime = -1.0;
    double _motionEndBeatTime = -1.0;

    LaneNotesTable const* _laneNotesTable = nullptr;
    // Only used in Multiphase
    LaneNotesTable const* _phaseNotesTable = nullptr;
    LaneNotesTable const* _deathNotesTable = nullptr;

    std::vector<BeatTimeEvent> _events;

    void OnShot(GameManager& g);
    void SendEventsFromLaneNoteTable(
        GameManager& g, LaneNotesTable const& laneNotesTable);
    void SendEventsFromEventsList(GameManager& g);
    int GetLaneIxFromCurrentPos();

    // _transform should contain the desired spawn position *before* calling
    // Init() for the spawn to work properly.
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    // virtual void Destroy(GameManager& g) override;
    virtual void OnEditPick(GameManager& g) override;

    // returns true if it can be hit by the player
    bool IsActive(GameManager& g) const;

protected:
    // Used by derived classes to save/load child-specific data.
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};
