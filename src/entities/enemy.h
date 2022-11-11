#pragma once

#include "new_entity.h"
#include "beat_time_event.h"
#include "input_manager.h"
#include "constants.h"

struct EnemyEntity : public ne::Entity {
    enum class Behavior {
        None,
        Down,
        Zigging,
    };
    enum class LaneNoteBehavior {
        None, // don't change note based on position/lane
        Minor, // Minor scale starting at _laneRootNote
        Table, // Use a lane-note mapping from a table
    };
    enum class OnHitBehavior {
        Default, // decrement hp, doot, die at 0 hp
        MultiPhase, // decrement hp, doot, increment phase at 0 hp, phase doot
    };
    static int constexpr kMaxNumNotesPerLane = 4;
    typedef std::array<std::array<int, kMaxNumNotesPerLane>, kNumLanes> LaneNotesTable;

    // serialized
    std::vector<BeatTimeEvent> _events;
    double _eventStartDenom = 0.125;
    InputManager::Key _shootButton = InputManager::Key::NumKeys;
    double _activeBeatTime = 0.0;
    double _inactiveBeatTime = -1.0;  // < 0 means it stays active once active.
    Behavior _behavior = Behavior::None;
    int _hp = -1;
    LaneNoteBehavior _laneNoteBehavior = LaneNoteBehavior::None;
    OnHitBehavior _onHitBehavior = OnHitBehavior::Default;
    // Down-specific
    float _downSpeed = 2.f;
    // LaneNote::Minor-specific
    int _laneRootNote = 36;  // C2
    std::vector<int> _laneNoteOffsets;
    // MultiPhase-specific
    // Note: _events is used for every hit
    int _numHpBars = 1;
    // std::vector<BeatTimeEvent> _phaseEvents;
    // std::vector<BeatTimeEvent> _deathEvents;
    int _channel = 0;
    double _noteLength = 0.25;


    float _desiredSpawnY = 0.f;
    double _shotBeatTime = -1.0;

    // specific to zigging
    bool _zigMoving = false;
    bool _zigLeft = true;
    Vec3 _zigSource;
    Vec3 _zigTarget;
    float _zigPhaseTime = 0.f;

    // specific to LaneNoteBehavior::Table
    LaneNotesTable const* _laneNotesTable = nullptr;

    void OnShot(GameManager& g);
    void SendEventsFromLaneNoteTable(GameManager& g);
    int GetLaneIxFromCurrentPos();

    // _transform should contain the desired spawn position *before* calling
    // Init() for the spawn to work properly.
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    // virtual void Destroy(GameManager& g) override;
    virtual void OnEditPick(GameManager& g) override;

    void SendEvents(GameManager& g, std::vector<BeatTimeEvent> const& events);

    // returns true if it can be hit by the player
    bool IsActive(GameManager& g) const;

protected:
    // Used by derived classes to save/load child-specific data.
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};
