#include "player_controller.h"

#include <limits>

#include "components/rigid_body.h"
#include "input_manager.h"
#include "components/transform.h"
#include "entity.h"
#include "game_manager.h"

bool PlayerControllerComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    bool success = true;
    _transform = e.FindComponentOfType<TransformComponent>();
    if (_transform.expired()) {
        success = false;
    }
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        success = false;
    } else {
        std::weak_ptr<PlayerControllerComponent> pComp =
            e.FindComponentOfType<PlayerControllerComponent>();
        _rb.lock()->AddOnHitCallback(
            std::bind(&PlayerControllerComponent::OnHit, pComp, std::placeholders::_1));
    }
    _input = g._inputManager;
    return success;
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
    RigidBodyComponent& rb = *_rb.lock();
    if (newState) {
        rb._layer = CollisionLayer::Solid;
    }

    // Check for attack transition
    if (_input->IsKeyPressedThisFrame(InputManager::Key::J) &&
        rb._velocity.Length2() > 0.1f*0.1f) {
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
        rb._velocity = Vec3(0.f,0.f,0.f);
        return false;
    }

    rb._velocity = inputVec.GetNormalized() * kIdleSpeed;

    return false;
}

bool PlayerControllerComponent::UpdateAttackState(float dt, bool newState) {
    RigidBodyComponent& rb = *_rb.lock();
    // triple the speed for a brief time, then decelerate.
    if (newState) {
        _stateTimer = 0.f;
        _attackDir = rb._velocity.GetNormalized();
        rb._velocity = _attackDir * kAttackSpeed;
        rb._layer = CollisionLayer::BodyAttack;
    }

    {
        float decelAmount = 175.f;
        Vec3 decel = -_attackDir * decelAmount;
        Vec3 newVel = rb._velocity + dt * decel;
        if (Vec3::Dot(newVel, _attackDir) <= 0.5*kIdleSpeed) {
            _state = State::Idle;
            return true;
        } else {
            rb._velocity = newVel;
        }
    }

    _stateTimer += dt;
    return false;
}

void PlayerControllerComponent::OnHit(
    std::weak_ptr<PlayerControllerComponent> thisComp,
    std::weak_ptr<RigidBodyComponent> /*other*/) {
    auto player = thisComp.lock();
    RigidBodyComponent& rb = *player->_rb.lock();
    player->_attackDir = -player->_attackDir;
    float currentSpeed = rb._velocity.Length();
    float newSpeed = std::max(30.f,currentSpeed);
    rb._velocity = player->_attackDir * newSpeed;
}