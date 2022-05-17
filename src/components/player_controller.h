#pragma once

#include <functional>

#include "component.h"
#include "matrix.h"

class RigidBodyComponent;
class InputManager;

class PlayerControllerComponent : public Component {
public:
    PlayerControllerComponent(TransformComponent* t, RigidBodyComponent* rb, InputManager const* input) 
        : _transform(t)
        , _rb(rb)
        , _input(input) {}
        
    virtual ~PlayerControllerComponent() {}

    virtual void Update(float dt) override;

    virtual void Destroy() override {}

    // Returns true if we need to re-evaluate state machine
    bool UpdateIdleState(float dt, bool newState);

    bool UpdateAttackState(float dt, bool newState);

    void OnHit();

    enum class State {
        Idle, Attacking
    };

    static inline float const kIdleSpeed = 5.f;
    static inline float const kAttackSpeed = 40.f;
    
    TransformComponent* _transform = nullptr;
    RigidBodyComponent* _rb = nullptr;
    InputManager const* _input = nullptr;
    State _state = State::Idle;
    float _stateTimer = 0.f;
    Vec3 _attackDir;
};