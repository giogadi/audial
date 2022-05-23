#pragma once

#include <functional>

#include "component.h"
#include "matrix.h"

class RigidBodyComponent;
class InputManager;

class PlayerControllerComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::PlayerController; }
    PlayerControllerComponent()
        : _attackDir(0.f,0.f,0.f) {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;
        
    virtual ~PlayerControllerComponent() {}

    virtual void Update(float dt) override;

    virtual void Destroy() override {}

    // Returns true if we need to re-evaluate state machine
    bool UpdateIdleState(float dt, bool newState);

    bool UpdateAttackState(float dt, bool newState);

    void OnHit(std::weak_ptr<RigidBodyComponent> other);

    enum class State {
        Idle, Attacking
    };

    static inline float const kIdleSpeed = 5.f;
    static inline float const kAttackSpeed = 40.f;
    
    std::weak_ptr<TransformComponent> _transform;
    std::weak_ptr<RigidBodyComponent> _rb;
    InputManager const* _input = nullptr;
    State _state = State::Idle;
    float _stateTimer = 0.f;
    Vec3 _attackDir;
};