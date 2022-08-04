#pragma once

#include <functional>

#include "component.h"
#include "matrix.h"

class InputManager;
class OrbitableComponent;
class TransformComponent;
class CameraControllerComponent;

class PlayerOrbitControllerComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::PlayerOrbitController; }
    PlayerOrbitControllerComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    virtual ~PlayerOrbitControllerComponent() {}

    virtual void Update(float dt) override;

    virtual void Destroy() override {}

    // Returns true if we need to re-evaluate state machine
    bool UpdateIdleState(float dt, bool newState);

    bool PickNextPlanetToOrbit(Vec3 const& inputVec, Vec3& dashDir);

    enum class State {
        Idle
    };

    EntityId _myId;
    std::weak_ptr<TransformComponent> _transform;
    std::weak_ptr<OrbitableComponent> _planetWeOrbit;
    EntityId _planetWeOrbitId;
    std::weak_ptr<CameraControllerComponent> _camera;
    InputManager const* _input = nullptr;
    GameManager* _g = nullptr;
    // For finding planets to orbit. Maybe we'll want a PlanetManager for this later.
    EntityManager* _entityMgr = nullptr;
    State _state = State::Idle;
    float _stateTimer = 0.f;
};