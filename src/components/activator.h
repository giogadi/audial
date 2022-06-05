#pragma once

#include <memory>

#include "component.h"

class GameManager;
class BeatClock;

class ActivatorComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Activator; }
    ActivatorComponent() {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;

    virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    // virtual void OnEditPick() override;

    std::string _entityName;
    GameManager* _gameManager;
    BeatClock const* _beatClock;
    // THIS IS TIME SINCE THE FIRST UPDATE() CALL ON THIS DUDE
    double _activationBeatTime = -1.0;
    bool _haveActivated = false;

    // THIS IS THE REAL TIME TO COMPARE TO BEATCLOCK
    double _absoluteActivationBeatTime = -1.0;
};