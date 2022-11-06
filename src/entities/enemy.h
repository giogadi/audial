#pragma once

#include "new_entity.h"
#include "beat_time_event.h"
#include "input_manager.h"

struct EnemyEntity : public ne::Entity {
    // serialized
    std::vector<BeatTimeEvent> _events;
    double _eventStartDenom = 0.125;
    InputManager::Key _shootButton = InputManager::Key::NumKeys;
    double _activeBeatTime = 0.0;  // read as an offset from the downbeat when this entity is initialized.
    double _inactiveBeatTime = -1.0;  // < 0 means it stays active once active.

    float _desiredSpawnY = 0.f;
    double _shotBeatTime = -1.0;

    void OnShot(GameManager& g);

    // _transform should contain the desired spawn position *before* calling Init() for the spawn to work properly.
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    // virtual void Destroy(GameManager& g) override;
    virtual void OnEditPick(GameManager& g) override;

    void SendEvents(GameManager& g);

    // returns true if it can be hit by the player
    bool IsActive(GameManager& g) const;

protected:
    // Used by derived classes to save/load child-specific data.
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};