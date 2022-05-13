#include "player_controller.h"

#include <limits>

#include "glm/gtc/constants.hpp"
#include "glm/gtx/norm.hpp"

#include "components/rigid_body.h"
#include "input_manager.h"

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
        glm::length2(_rb->_velocity) > 0.1f*0.1f) {
        _state = State::Attacking;
        return true;
    }

    glm::vec3 inputVec(0.0f);
    if (_input->IsKeyPressed(InputManager::Key::W)) {
        inputVec.z -= 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::S)) {
        inputVec.z += 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::A)) {
        inputVec.x -= 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::D)) {
        inputVec.x += 1.0f;
    }

    if (inputVec.x == 0.f && inputVec.y == 0.f && inputVec.z == 0.f) {
        _rb->_velocity = glm::vec3(0.f);
        return false;
    }

    _rb->_velocity = glm::normalize(inputVec) * kIdleSpeed;
    
    return false;
}

bool PlayerControllerComponent::UpdateAttackState(float dt, bool newState) {
    // triple the speed for a brief time, then decelerate.
    if (newState) {
        _stateTimer = 0.f;
        _attackDir = glm::normalize(_rb->_velocity);
        _rb->_velocity = _attackDir * kAttackSpeed;
    }

    {
        float decelAmount = 175.f;
        glm::vec3 decel = -_attackDir * decelAmount;
        glm::vec3 newVel = _rb->_velocity + dt * decel;
        if (glm::dot(newVel, _attackDir) <= 0.5*kIdleSpeed) {
            // _rb->_velocity = glm::vec3(0.f);    
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
    float currentSpeed = glm::length(_rb->_velocity);
    float newSpeed = std::max(30.f,currentSpeed);
    _rb->_velocity = _attackDir * newSpeed;
}