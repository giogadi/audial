#include "player_orbit_controller.h"

#include <limits>

#include "components/rigid_body.h"
#include "input_manager.h"
#include "components/beep_on_hit.h"
#include "constants.h"

namespace {
    float constexpr kIdleSpeed = 10.f;
    float constexpr kAttackSpeed = 60.f;
    float constexpr kDecel = 125.f;
    float constexpr kOrbitRange = 5.f;
    float constexpr kOrbitSpeed = 2*kPi;  // rads per second
}

bool PlayerOrbitControllerComponent::ConnectComponents(Entity& e, GameManager& g) {
    bool success = true;
    _transform = e.FindComponentOfType<TransformComponent>();
    if (_transform.expired()) {
        success = false;
    }
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        success = false;
    } else {
        std::weak_ptr<PlayerOrbitControllerComponent> pComp =
            e.FindComponentOfType<PlayerOrbitControllerComponent>();
        _rb.lock()->SetOnHitCallback(
            std::bind(&PlayerOrbitControllerComponent::OnHit, pComp, std::placeholders::_1));
    }    
    _input = g._inputManager;
    _entityMgr = g._entityManager;
    return success;
}

void PlayerOrbitControllerComponent::Update(float dt) {
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

namespace {
    // For now, we're looking for BeepOnHit, but we'll probably want a Planet component
    std::weak_ptr<Entity> FindClosestPlanetInRange(
        EntityManager const* entityMgr, float const range, Vec3 const& playerPos) {
        
        std::weak_ptr<Entity> closest;
        float closestDist2 = range * range;
        for (std::shared_ptr<Entity> const& e : entityMgr->_entities) {
            std::shared_ptr<BeepOnHitComponent> planet = e->FindComponentOfType<BeepOnHitComponent>().lock();
            if (planet == nullptr) {
                continue;
            }
            Vec3 const& planetPos = planet->_t.lock()->GetPos();
            Vec3 fromPlayerToPlanet = planetPos - playerPos;
            float dist2 = fromPlayerToPlanet.Length2();
            if (dist2 <= closestDist2) {
                closestDist2 = dist2;
                closest = e;
            }
        }

        return closest;
    }

    Vec3 GetInputVec(InputManager const& input) {
        Vec3 inputVec(0.0f,0.f,0.f);
            if (input.IsKeyPressed(InputManager::Key::W)) {
                inputVec._z -= 1.0f;
            }
            if (input.IsKeyPressed(InputManager::Key::S)) {
                inputVec._z += 1.0f;
            }
            if (input.IsKeyPressed(InputManager::Key::A)) {
                inputVec._x -= 1.0f;
            }
            if (input.IsKeyPressed(InputManager::Key::D)) {
                inputVec._x += 1.0f;
            }

            return inputVec.GetNormalized();
    }
}

namespace {
    float XZToAngle(float x, float z) {
        return atan2(-z, x);
    }
    void AngleToXZ(float radians, float& x, float& z) {
        x = cos(radians);
        z = -sin(radians);
    }
}

bool PlayerOrbitControllerComponent::UpdateIdleState(float dt, bool newState) {
    Vec3 const inputVec = GetInputVec(*_input);

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

    if (std::shared_ptr<BeepOnHitComponent> planet = _planetWeOrbit.lock()) {
        // Gradually push/pull ourselves into the desired range of the planet.
        Vec3 const playerPos = _transform.lock()->GetPos();
        Vec3 const planetPos = planet->_t.lock()->GetPos();
        Vec3 planetToPlayer = playerPos - planetPos;
        float currentRange = planetToPlayer.Length();
        float kDesiredRange = 3.f;
        // get current angle with planet
        // These 2D angles are defined about the y-axis, where angle=0 is the +x axis and angle=pi/2 is the -z axis.
        float currentAngle = XZToAngle(planetToPlayer._x, planetToPlayer._z);
        float newAngle = currentAngle + kOrbitSpeed*dt;        
        float newRange = currentRange + 0.05f*(kDesiredRange - currentRange);
        Vec3 newPos(0.f, 0.f, 0.f);
        AngleToXZ(newAngle, newPos._x, newPos._z);
        newPos = newPos*newRange + planetPos;
        rb._velocity = (newPos - playerPos).GetNormalized();
        rb._velocity *= kIdleSpeed;
        return false;
    }
    
    if (inputVec.IsZero()) {
        rb._velocity = Vec3(0.f,0.f,0.f);
        return false;
    }

    rb._velocity = inputVec * kIdleSpeed;
    
    return false;
}

bool PlayerOrbitControllerComponent::PickNextPlanetToOrbit(Vec3 const& inputVec, Vec3& dashDir) {
    Vec3 playerPos = _transform.lock()->GetPos();
    if (inputVec.IsZero()) {
        // Don't change planets
        auto currentPlanet = _planetWeOrbit.lock();
        if (currentPlanet) {
            Vec3 const& planetPos = currentPlanet->_t.lock()->GetPos();
            dashDir = (planetPos - playerPos).GetNormalized();
            return true;
        } else {
            return false;
        }        
    }

    // WE USE CURRENTLY ORBITED PLANET AS REFERENCE, NOT PLAYER
    float constexpr kMinDotWithInput = 0.707106781f;
    float closestDist = std::numeric_limits<float>::max();
    dashDir = inputVec;
    std::shared_ptr<BeepOnHitComponent> currentPlanet = _planetWeOrbit.lock();
    std::shared_ptr<BeepOnHitComponent> potentialNewPlanet;
    if (currentPlanet) {
        playerPos = currentPlanet->_t.lock()->GetPos();
    }
    for (std::shared_ptr<Entity> const& e : _entityMgr->_entities) {
        std::shared_ptr<BeepOnHitComponent> planet = e->FindComponentOfType<BeepOnHitComponent>().lock();
        if (planet == nullptr) {
            continue;
        }
        if (planet == currentPlanet) {
            continue;
        }
        Vec3 const& planetPos = planet->_t.lock()->GetPos();
        Vec3 fromPlayerToPlanetDir = planetPos - playerPos;
        float dist = fromPlayerToPlanetDir.Length();
        if (dist == 0.f) {
            continue;
        }
        fromPlayerToPlanetDir *= (1.f / dist);
        float dotWithInput = Vec3::Dot(fromPlayerToPlanetDir, inputVec);
        if (dotWithInput < kMinDotWithInput) {
            continue;
        }

        if (dist < closestDist) {            
            closestDist = dist;        
            potentialNewPlanet = planet;
            dashDir = (planetPos - _transform.lock()->GetPos()).GetNormalized();
        }
    }

    if (currentPlanet) {
        currentPlanet->_rb.lock()->_layer = CollisionLayer::None;
    }
    if (potentialNewPlanet) {
        potentialNewPlanet->_rb.lock()->_layer = CollisionLayer::Solid;
    }    
    _planetWeOrbit = potentialNewPlanet;

    return true;
}

bool PlayerOrbitControllerComponent::UpdateAttackState(float dt, bool newState) {
    RigidBodyComponent& rb = *_rb.lock();
    // triple the speed for a brief time, then decelerate.
    // Also pick the new planet to orbit
    if (newState || _input->IsKeyPressedThisFrame(InputManager::Key::J)) {
        _stateTimer = 0.f;
        // TODO: compute inputVec only once before state machine
        Vec3 const inputVec = GetInputVec(*_input);

        Vec3 dashDir(0.f,0.f,0.f);
        if (!PickNextPlanetToOrbit(inputVec, dashDir)) {
            // Not dashing. just skip this update this frame and then we'll go
            // back to idle. NOTE: we don't go immediately back to idle here
            // because it might cause an infinite loop of the state machine,
            // lmao.
            _state = State::Idle;
            return false;
        }

        _attackDir = dashDir;
        if (inputVec.IsZero()) {
            _dribbleRadialSpeed = -kAttackSpeed;
        } else {
            _dribbleRadialSpeed.reset();
            rb._velocity = _attackDir * kAttackSpeed;
        }

        rb._layer = CollisionLayer::BodyAttack;
    }

    if (_dribbleRadialSpeed.has_value()) {
        Vec3 const planetPos = _planetWeOrbit.lock()->_t.lock()->GetPos();
        Vec3 const playerPos = _transform.lock()->GetPos();
        Vec3 const planetToPlayer = playerPos - planetPos;        

        float currentAngle = XZToAngle(planetToPlayer._x, planetToPlayer._z);
        float newAngle = currentAngle + kOrbitSpeed*dt; 
        if (_dribbleRadialSpeed.value() > 0) {
            *_dribbleRadialSpeed -= kDecel * dt;
        } else {
            *_dribbleRadialSpeed += kDecel * dt;
        }
        if (fabs(*_dribbleRadialSpeed) < 0.5f*kIdleSpeed) {
            _state = State::Idle;
            _dribbleRadialSpeed.reset();
            return true;
        }

        // 
        Vec3 newPlanetToPlayerDir(0.f,0.f,0.f);
        AngleToXZ(newAngle, newPlanetToPlayerDir._x, newPlanetToPlayerDir._z);        
        Vec3 playerRadialVel = newPlanetToPlayerDir * _dribbleRadialSpeed.value();

        float currentRadius = planetToPlayer.Length();
        float newRadius = currentRadius + *_dribbleRadialSpeed * dt;
        Vec3 tangentDir = Vec3::Cross(Vec3(0.f,1.f,0.f), newPlanetToPlayerDir);
        Vec3 playerTangentVel = newRadius * kOrbitSpeed * tangentDir;

        // this part ensures that our overall velocity tracks the general
        // accel-decel curve on dribbleRadialSpeed. This makes dribbles feel
        // consistent with regular dashes.
        rb._velocity = playerRadialVel + playerTangentVel;
        float maxVel = std::min(rb._velocity.Length(), fabs(*_dribbleRadialSpeed));
        rb._velocity = rb._velocity.GetNormalized() * maxVel;
    } else {
        // Non-dribble
        Vec3 decel = -_attackDir * kDecel;
        Vec3 newVel = rb._velocity + dt * decel;
        if (Vec3::Dot(newVel, _attackDir) <= 0.5f*kIdleSpeed) {
            _state = State::Idle;            
            return true;
        } else {
            rb._velocity = newVel;
        }
    }

    _stateTimer += dt;
    return false;
}

void PlayerOrbitControllerComponent::OnHit(
    std::weak_ptr<PlayerOrbitControllerComponent> thisComp,
    std::weak_ptr<RigidBodyComponent> /*other*/) {
    auto player = thisComp.lock();
    RigidBodyComponent& rb = *player->_rb.lock();
    if (player->_dribbleRadialSpeed.has_value()) {
        player->_dribbleRadialSpeed = 30.f;
        // TODO: add tangent component of speed here
        // TODO: why do we need to update velocity AND dribble radial / attackdir?
        Vec3 const planetToPlayerDir =
            (player->_transform.lock()->GetPos() - player->_planetWeOrbit.lock()->_t.lock()->GetPos()).GetNormalized();
        rb._velocity = planetToPlayerDir * player->_dribbleRadialSpeed.value();
    } else {
        player->_attackDir = -player->_attackDir;        
        // float currentSpeed = rb._velocity.Length();
        // float newSpeed = std::max(30.f,currentSpeed);
        float newSpeed = 30.f;
        rb._velocity = player->_attackDir * newSpeed;
    }        
}