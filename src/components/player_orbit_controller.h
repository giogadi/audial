#pragma once

#include <functional>

#include "component.h"
#include "matrix.h"

class RigidBodyComponent;
class InputManager;
class BeepOnHitComponent;

class PlayerOrbitControllerComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::PlayerOrbitController; }
    PlayerOrbitControllerComponent()
        : _attackDir(0.f,0.f,0.f) {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;
        
    virtual ~PlayerOrbitControllerComponent() {}

    virtual void Update(float dt) override;

    virtual void Destroy() override {}

    // Returns true if we need to re-evaluate state machine
    bool UpdateIdleState(float dt, bool newState);

    bool UpdateAttackState(float dt, bool newState);

    bool PickNextPlanetToOrbit(Vec3 const& inputVec, Vec3& dashDir);

    static void OnHit(
        std::weak_ptr<PlayerOrbitControllerComponent> thisComp, std::weak_ptr<RigidBodyComponent> other);

    enum class State {
        Idle, Attacking
    };
    
    std::weak_ptr<TransformComponent> _transform;
    std::weak_ptr<RigidBodyComponent> _rb;
    std::weak_ptr<BeepOnHitComponent> _planetWeOrbit;
    InputManager const* _input = nullptr;
    // For finding planets to orbit. Maybe we'll want a PlanetManager for this later.
    EntityManager const* _entityMgr = nullptr;
    State _state = State::Idle;
    float _stateTimer = 0.f;
    Vec3 _attackDir;
    // negative is moving closer to center.
    std::optional<float> _dribbleRadialSpeed;
};