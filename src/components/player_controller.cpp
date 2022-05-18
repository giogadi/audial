#include "player_controller.h"

#include <limits>

#include "components/rigid_body.h"
#include "input_manager.h"

void PlayerControllerComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.DebugFindComponentOfType<TransformComponent>();
    _rb = e.DebugFindComponentOfType<RigidBodyComponent>();
    _rb->SetOnHitCallback(std::bind(&PlayerControllerComponent::OnHit, this));
    _input = g._inputManager;
}

void PlayerControllerComponent::Update(float dt) {
    bool evalStateMachine = true;
    State prevState = _state;
    while (evalStateMachine) {
        bool newState = _state != prevState;
        prevState = _state;
        switch (_state) {
            case State::Idle:
                evalStateMachine = UpdateIdleState(dt, newState);
                break;
            case State::Attacking:
                evalStateMachine = UpdateAttackState(dt, newState);
                break;
        }
    }
}

bool PlayerControllerComponent::UpdateIdleState(float dt, bool newState) {
    // Check for attack transition
    if (_input->IsKeyPressedThisFrame(InputManager::Key::J) &&
        _rb->_velocity.Length2() > 0.1f*0.1f) {
        _state = State::Attacking;
        return true;
    }

    Vec3 inputVec(0.0f,0.f,0.f);
    if (_input->IsKeyPressed(InputManager::Key::W)) {
        inputVec._z -= 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::S)) {
        inputVec._z += 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::A)) {
        inputVec._x -= 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::D)) {
        inputVec._x += 1.0f;
    }

    if (inputVec._x == 0.f && inputVec._y == 0.f && inputVec._z == 0.f) {
        _rb->_velocity = Vec3(0.f,0.f,0.f);
        return false;
    }

    _rb->_velocity = inputVec.GetNormalized() * kIdleSpeed;
    
    return false;
}

bool PlayerControllerComponent::UpdateAttackState(float dt, bool newState) {
    // triple the speed for a brief time, then decelerate.
    if (newState) {
        _stateTimer = 0.f;
        _attackDir = _rb->_velocity.GetNormalized();
        _rb->_velocity = _attackDir * kAttackSpeed;
    }

    {
        float decelAmount = 175.f;
        Vec3 decel = -_attackDir * decelAmount;
        Vec3 newVel = _rb->_velocity + dt * decel;
        if (Vec3::Dot(newVel, _attackDir) <= 0.5*kIdleSpeed) {
            _state = State::Idle;            
            return true;
        } else {
            _rb->_velocity = newVel;
        }
    }

    _stateTimer += dt;
    return false;
}

void PlayerControllerComponent::OnHit() {
    _attackDir = -_attackDir;
    float currentSpeed = _rb->_velocity.Length();
    float newSpeed = std::max(30.f,currentSpeed);
    _rb->_velocity = _attackDir * newSpeed;
}