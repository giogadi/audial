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
    float _motionAngleDegrees = 0.0f;  // +x is 0 degrees, + is ccw about y axis
    float _motionSpeed = 0.f;

    double _activeBeatTimeAbsolute = 0.0;

    void OnShot(GameManager& g);

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    // virtual void Destroy(GameManager& g) override;
    virtual void OnEditPick(GameManager& g) override;

    void SendEvents(GameManager& g);
    bool IsActive(GameManager& g) const;

protected:
    // Used by derived classes to save/load child-specific data.
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};