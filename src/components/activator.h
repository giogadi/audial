#pragma once

#include "component.h"

struct GameManager;
class BeatClock;

class ActivatorComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Activator; }
    ActivatorComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    void Save(serial::Ptree pt) const override;
    void Load(serial::Ptree pt) override;

    // Serialize
    std::string _entityName;
    // THIS IS TIME SINCE THE FIRST UPDATE() CALL ON THIS DUDE
    double _activationBeatTime = -1.0;

    GameManager* _gameManager;
    BeatClock const* _beatClock;
    bool _haveActivated = false;

    // THIS IS THE REAL TIME TO COMPARE TO BEATCLOCK
    double _absoluteActivationBeatTime = -1.0;
};