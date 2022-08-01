#include "player_orbit_controller.h"

#include <limits>

#include "components/rigid_body.h"
#include "input_manager.h"
#include "components/orbitable.h"
#include "constants.h"
#include "components/transform.h"
#include "entity_manager.h"
#include "game_manager.h"
#include "components/camera_controller.h"

namespace {
    float constexpr kIdleSpeed = 10.f;
    float constexpr kAttackSpeed = 60.f;
    float constexpr kDecel = 125.f;
    float constexpr kOrbitRange = 5.f;
    float constexpr kOrbitAngularSpeed = 2*kPi;  // rads per second
    // float constexpr kOrbitAngularSpeed = 0.f;  // rads per second
    float constexpr kDesiredRange = 3.f;
    float constexpr kIdleSpeedTowardDesiredRange = 20.f;
    // float constexpr kIdleSpeedTowardDesiredRange = 5.f;
}

bool PlayerOrbitControllerComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    bool success = true;
    _transform = e.FindComponentOfType<TransformComponent>();
    if (_transform.expired()) {
        success = false;
    }
    // _rb = e.FindComponentOfType<RigidBodyComponent>();
    // if (_rb.expired()) {
    //     success = false;
    // } else {
    //     std::weak_ptr<PlayerOrbitControllerComponent> pComp =
    //         e.FindComponentOfType<PlayerOrbitControllerComponent>();
    //     _rb.lock()->AddOnHitCallback(
    //         std::bind(&PlayerOrbitControllerComponent::OnHit, pComp, std::placeholders::_1));
    // }
    _input = g._inputManager;
    _entityMgr = g._entityManager;
    _g = &g;
    // Find a camera controller if it exists
    // TODO THIS IS GROSS
    _entityMgr->ForEveryActiveEntity([&](EntityId eId) {
        Entity* e = _entityMgr->GetEntity(eId);
        assert(e);
        auto pComp = e->FindComponentOfType<CameraControllerComponent>();
        if (!pComp.expired()) {
            _camera = pComp;
        }
    });
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
    bool WasDashKeyPressedThisFrame(InputManager const& input) {
        return input.IsKeyPressedThisFrame(InputManager::Key::W) ||
            input.IsKeyPressedThisFrame(InputManager::Key::S) ||
            input.IsKeyPressedThisFrame(InputManager::Key::A) ||
            input.IsKeyPressedThisFrame(InputManager::Key::D) ||
            input.IsKeyPressedThisFrame(InputManager::Key::J);
    }
}

static bool gApproaching = false;

bool PlayerOrbitControllerComponent::UpdateIdleState(float dt, bool newState) {
    if (dt == 0.f) {
        return false;
    }

    Vec3 const inputVec = GetInputVec(*_input);

    if (WasDashKeyPressedThisFrame(*_input)) {
        Vec3 const inputVec = GetInputVec(*_input);
        Vec3 dashDir;
        PickNextPlanetToOrbit(inputVec, dashDir);
        if (std::shared_ptr<OrbitableComponent> planet = _planetWeOrbit.lock()) {            
            gApproaching = true;
            std::shared_ptr<TransformComponent> playerTrans = _transform.lock();
            if (std::shared_ptr<TransformComponent const> currentParent = playerTrans->_parent.lock()) {
                if (currentParent != planet->_t.lock()) {
                    playerTrans->Unparent();
                    // When we unparent from a planet, the camera goes back to following the player.
                    if (std::shared_ptr<CameraControllerComponent> camera = _camera.lock()) {
                        camera->SetTarget(playerTrans);
                    }
                }
            }
        }
    }

    if (std::shared_ptr<OrbitableComponent> planet = _planetWeOrbit.lock()) {            
        std::shared_ptr<TransformComponent> playerTrans = _transform.lock();
        std::shared_ptr<TransformComponent> planetTrans = planet->_t.lock();
        
        float constexpr kPlayerRadius = 0.5f;
        float constexpr kPlanetRadius = 0.5f;
        float constexpr kMinDist = kPlayerRadius + kPlanetRadius;
        if (std::shared_ptr<TransformComponent const> parent = playerTrans->_parent.lock()) {
            // Decide new radius
            Vec3 const& playerLocalPos = playerTrans->GetLocalPos();
            Vec3 planetToPlayerDirLocal = playerTrans->GetLocalPos();
            float currentRadius = planetToPlayerDirLocal.Normalize();
            float newRadius = currentRadius;
            if (gApproaching) {
                float lagFactor = 0.2f;                
                // 1st order lag to desired radius.
                float desiredRad = 0.f;
                newRadius = currentRadius + lagFactor * (desiredRad - currentRadius);
                if (newRadius < kMinDist) {
                    newRadius = kMinDist;
                    gApproaching = false;                    
                }
            } else {
                float lagFactor = 0.2f;
                // 1st order lag to desired radius.
                newRadius = currentRadius + lagFactor * (kDesiredRange - currentRadius);
            }

            // Now decide new angle.
            // These 2D angles are defined about the y-axis, where angle=0 is the +x axis and angle=pi/2 is the -z axis.
            float currentAngle = XZToAngle(planetToPlayerDirLocal._x, planetToPlayerDirLocal._z);
            float newAngle = currentAngle + kOrbitAngularSpeed * dt;
            Vec3 newPlanetToPlayerDir;
            AngleToXZ(newAngle, newPlanetToPlayerDir._x, newPlanetToPlayerDir._z);

            Vec3 newLocalPos = newPlanetToPlayerDir * newRadius;
            playerTrans->SetLocalPos(newLocalPos);
        } else {
            // No parent. We better be approaching!
            assert(gApproaching);
            float lagFactor = 0.2f;
            Vec3 const& playerPos = playerTrans->GetWorldPos();
            Vec3 const& planetPos = planetTrans->GetWorldPos();       
            Vec3 playerToPlanet = planetPos - playerPos;
            Vec3 newPos = playerPos + lagFactor * playerToPlanet;
            Vec3 planetToNewPos = newPos - planetPos;
            float newDist = planetToNewPos.Normalize();
            if (newDist < kMinDist) {
                newPos = planetPos + planetToNewPos * kMinDist;
                playerTrans->Parent(planetTrans);
                gApproaching = false;
                // When we parent to a new planet, the camera controller should follow the planet.
                if (std::shared_ptr<CameraControllerComponent> camera = _camera.lock()) {
                    camera->SetTarget(planetTrans);
                }
            }
            playerTrans->SetWorldPos(newPos);
        }
    }

    return false;
}

bool PlayerOrbitControllerComponent::UpdateIdleStateOld(float dt, bool newState) {    
    Vec3 const inputVec = GetInputVec(*_input);

    RigidBodyComponent& rb = *_rb.lock();
    if (newState) {
        rb._layer = CollisionLayer::Solid;
    }

    // Check for attack transition
    // HOWDY
    // if (WasDashKeyPressedThisFrame(*_input)) {
    //     _state = State::Attacking;
    //     return true;
    // }

    if (WasDashKeyPressedThisFrame(*_input)) {
        Vec3 const inputVec = GetInputVec(*_input);
        Vec3 dashDir;
        PickNextPlanetToOrbit(inputVec, dashDir);
        if (std::shared_ptr<OrbitableComponent> planet = _planetWeOrbit.lock()) {
            TransformComponent& playerTransform = *_transform.lock();
            std::shared_ptr<TransformComponent> pPlanetTransform = planet->_t.lock();
            gApproaching = true;
            playerTransform.Parent(pPlanetTransform);
        }        
    }

    if (std::shared_ptr<OrbitableComponent> planet = _planetWeOrbit.lock()) {
        //
        // NEWEST WITH PARENTING
        //
        // We approach the planet radially first, and once we're within the
        // desired distance we start rotating. Later we'll deal with the hit and
        // bounce.
        std::shared_ptr<TransformComponent> playerTrans = _transform.lock();
        Vec3 planetToPlayerLocal = playerTrans->GetLocalPos();
        float const distFromPlanet = planetToPlayerLocal.Normalize();
        if (gApproaching /*&& distFromPlanet > kDesiredRange*/) {
            // Set velocity in local space. Later, collisions will consider it in local space.
            rb._velocity = -planetToPlayerLocal * kIdleSpeedTowardDesiredRange;
        } else {
            gApproaching = false;
            // Start rotating around.
            Vec3 orbitDir = Vec3::Cross(-planetToPlayerLocal, Vec3(0.f, 1.f, 0.f));
            Vec3 orbitVel = orbitDir * (distFromPlanet * kOrbitAngularSpeed);
            rb._velocity = orbitVel;

            // Also add a vel component to maintain the distance at desiredRange. Mitigates drift.
            float rangeDiff = distFromPlanet - kDesiredRange;  // + if farther than desired.
            float correctionSpeed;
            float stepSize = kIdleSpeedTowardDesiredRange * dt;
            if (stepSize > std::abs(rangeDiff)) {
                correctionSpeed = rangeDiff / dt;
            } else {
                correctionSpeed = ((rangeDiff > 0.f) ? 1.f : -1.f) * kIdleSpeedTowardDesiredRange;
            }

            // float correctionSpeed = rangeDiff / dt;
            Vec3 correctionVel = -correctionSpeed * planetToPlayerLocal;
            rb._velocity += correctionVel;
        }
        return false;

        //
        // NEWER UPDATE
        //

        //
        // First select the velocity for getting to the desired range.
        //

        // Vec3 velTowardRange;
        // Vec3 const playerPos = _transform.lock()->GetWorldPos();
        // Vec3 const planetPos = planet->_t.lock()->GetWorldPos();
        // Vec3 playerToPlanet = planetPos - playerPos;
        // float const currentRange = playerToPlanet.Length();
        // playerToPlanet = playerToPlanet / currentRange;
        // float stepSize = kIdleSpeedTowardDesiredRange * dt;
        // if (currentRange > kDesiredRange) {
        //     if (stepSize > currentRange - kDesiredRange) {
        //         velTowardRange = (playerToPlanet * ((currentRange - kDesiredRange) / dt));
        //     } else {
        //         velTowardRange = kIdleSpeedTowardDesiredRange * playerToPlanet;
        //     }
        // } else {
        //     if (stepSize > kDesiredRange - currentRange) {
        //         velTowardRange = (playerToPlanet * ((currentRange - kDesiredRange) / dt));
        //     } else {
        //         velTowardRange = -kIdleSpeedTowardDesiredRange * playerToPlanet;
        //     }
        // }

        // //
        // // NOW let's find the vel component from orbit angular velocity
        // //
        // Vec3 orbitDir = Vec3::Cross(playerToPlanet, Vec3(0.f, 1.f, 0.f));
        // Vec3 orbitVel = orbitDir * (currentRange * kOrbitAngularSpeed);

        // rb._velocity = velTowardRange + orbitVel;

        // return false;
        
        //
        // OLD UPDATE
        //

        // Gradually push/pull ourselves into the desired range of the planet.
        // Vec3 const playerPos = _transform.lock()->GetWorldPos();
        // Vec3 const planetPos = planet->_t.lock()->GetWorldPos();
        // Vec3 planetToPlayer = playerPos - planetPos;
        // float currentRange = planetToPlayer.Length();
        // // get current angle with planet
        // // These 2D angles are defined about the y-axis, where angle=0 is the +x axis and angle=pi/2 is the -z axis.
        // float currentAngle = XZToAngle(planetToPlayer._x, planetToPlayer._z);
        // float newAngle = currentAngle + kOrbitSpeed*dt;
        // float newRange = currentRange + 0.05f*(kDesiredRange - currentRange);
        // Vec3 newPos(0.f, 0.f, 0.f);
        // AngleToXZ(newAngle, newPos._x, newPos._z);
        // newPos = newPos*newRange + planetPos;
        // rb._velocity = (newPos - playerPos).GetNormalized();
        // rb._velocity *= kIdleSpeed;
        // return false;
    }

    if (inputVec.IsZero()) {
        rb._velocity = Vec3(0.f,0.f,0.f);
        return false;
    }

    rb._velocity = inputVec * kIdleSpeed;

    return false;
}

bool PlayerOrbitControllerComponent::PickNextPlanetToOrbit(Vec3 const& inputVec, Vec3& dashDir) {
    Vec3 playerPos = _transform.lock()->GetWorldPos();
    if (inputVec.IsZero()) {
        // Don't change planets
        auto currentPlanet = _planetWeOrbit.lock();
        if (currentPlanet) {
            Vec3 const& planetPos = currentPlanet->_t.lock()->GetWorldPos();
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
    std::shared_ptr<OrbitableComponent> currentPlanet = _planetWeOrbit.lock();
    std::shared_ptr<OrbitableComponent> potentialNewPlanet;
    if (currentPlanet) {
        playerPos = currentPlanet->_t.lock()->GetWorldPos();
    }
    _entityMgr->ForEveryActiveEntity([&](EntityId id) {
        Entity& e = *_entityMgr->GetEntity(id);
        std::shared_ptr<OrbitableComponent> planet = e.FindComponentOfType<OrbitableComponent>().lock();
        if (planet == nullptr) {
            return;
        }
        if (planet == currentPlanet) {
            return;
        }
        Vec3 const& planetPos = planet->_t.lock()->GetWorldPos();
        Vec3 fromPlayerToPlanetDir = planetPos - playerPos;
        float dist = fromPlayerToPlanetDir.Length();
        if (dist == 0.f) {
            return;
        }
        fromPlayerToPlanetDir *= (1.f / dist);
        float dotWithInput = Vec3::Dot(fromPlayerToPlanetDir, inputVec);
        if (dotWithInput < kMinDotWithInput) {
            return;
        }

        if (dist < closestDist) {
            closestDist = dist;
            potentialNewPlanet = planet;
            dashDir = (planetPos - _transform.lock()->GetWorldPos()).GetNormalized();
        }
    });

    if (currentPlanet) {
        currentPlanet->_rb.lock()->_layer = CollisionLayer::None;
        currentPlanet->OnLeaveOrbit(*_g);
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
    if (newState || WasDashKeyPressedThisFrame(*_input)) {
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
        Vec3 const planetPos = _planetWeOrbit.lock()->_t.lock()->GetWorldPos();
        Vec3 const playerPos = _transform.lock()->GetWorldPos();
        Vec3 const planetToPlayer = playerPos - planetPos;

        float currentAngle = XZToAngle(planetToPlayer._x, planetToPlayer._z);
        float newAngle = currentAngle + kOrbitAngularSpeed*dt;
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
        Vec3 playerTangentVel = newRadius * kOrbitAngularSpeed * tangentDir;

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
    gApproaching = false;
    rb._velocity.Set(0.f, 0.f, 0.f);
    // if (player->_dribbleRadialSpeed.has_value()) {
    //     player->_dribbleRadialSpeed = 30.f;
    //     // TODO: add tangent component of speed here
    //     // TODO: why do we need to update velocity AND dribble radial / attackdir?
    //     Vec3 const planetToPlayerDir =
    //         (player->_transform.lock()->GetWorldPos() - player->_planetWeOrbit.lock()->_t.lock()->GetWorldPos()).GetNormalized();
    //     rb._velocity = planetToPlayerDir * player->_dribbleRadialSpeed.value();
    // } else {
    //     player->_attackDir = -player->_attackDir;
    //     // float currentSpeed = rb._velocity.Length();
    //     // float newSpeed = std::max(30.f,currentSpeed);
    //     float newSpeed = 30.f;
    //     rb._velocity = player->_attackDir * newSpeed;
    // }
}