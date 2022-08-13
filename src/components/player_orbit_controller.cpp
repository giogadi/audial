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
#include "components/events_on_hit.h"

namespace {
    float constexpr kOrbitRange = 5.f;
    float constexpr kOrbitAngularSpeed = 2*kPi;  // rads per second
    float constexpr kDesiredRange = 3.f;


}

bool PlayerOrbitControllerComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    bool success = true;
    _myId = id;
    _transform = e.FindComponentOfType<TransformComponent>();
    if (_transform.expired()) {
        success = false;
    }
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

static EntityId gDashTargetPlanetId;
static Vec3 gLastVelocity;
static Vec3 gLastWorldPos;

bool PlayerOrbitControllerComponent::UpdateIdleState(float dt, bool newState) {
    if (dt == 0.f) {
        return false;
    }

    std::shared_ptr<TransformComponent> playerTrans = _transform.lock();
    Entity* planetWeOrbitEntity = _entityMgr->GetEntity(_planetWeOrbitId);
    std::shared_ptr<OrbitableComponent> planetWeOrbit;
    if (planetWeOrbitEntity != nullptr) {
        planetWeOrbit = planetWeOrbitEntity->FindComponentOfType<OrbitableComponent>().lock();
        assert(planetWeOrbit != nullptr);
    }

    if (WasDashKeyPressedThisFrame(*_input)) {
        Vec3 const inputVec = GetInputVec(*_input);           
        EntityId nextPlanetId = GetNextPlanetFromInput(inputVec);
        if (nextPlanetId != _planetWeOrbitId) {
            playerTrans->Unparent();
            // When we unparent from a planet, the camera goes back to following the player.
            if (std::shared_ptr<CameraControllerComponent> camera = _camera.lock()) {
                camera->SetTarget(playerTrans);
            }

            if (_input->IsKeyPressed(InputManager::Key::K) && planetWeOrbit != nullptr) {
                // Keep same orbiting planet.
            } else {
                if (planetWeOrbit != nullptr) {
                    planetWeOrbit->OnLeaveOrbit(*_g);                    
                }
                _planetWeOrbitId = nextPlanetId;
            }
        }

        gDashTargetPlanetId = nextPlanetId;
    }

    float constexpr kPlayerRadius = 0.5f;
    float constexpr kPlanetRadius = 0.5f;
    float constexpr kMinDist = kPlayerRadius + kPlanetRadius;

    // Dashing to a planet can happen in 2 ways:
    // 1. If we are parented to the planet (i.e., we orbit them AND we've already hit them once), we just vary the radius.
    // 2. If we are not parented to that planet, we do a pure first-order-lag to the planet's position.
    if (Entity* dashTargetPlanet = _entityMgr->GetEntity(gDashTargetPlanetId)) {
        std::shared_ptr<OrbitableComponent> planetOrbitable = dashTargetPlanet->FindComponentOfType<OrbitableComponent>().lock();
        assert(planetOrbitable != nullptr);
        std::shared_ptr<TransformComponent> planetTrans = dashTargetPlanet->FindComponentOfType<TransformComponent>().lock();
        assert(planetTrans != nullptr);

        if (std::shared_ptr<TransformComponent const> playerParent = playerTrans->_parent.lock()) {
            assert(playerParent == planetTrans);

            // Decide new radius
            Vec3 planetToPlayerDirLocal = playerTrans->GetLocalPos();
            float currentRadius = planetToPlayerDirLocal.Normalize();
            float newRadius = currentRadius;
            {
                float lagFactor = 0.3f * 60.f;                
                // 1st order lag to desired radius.
                float desiredRad = 0.f;
                newRadius = currentRadius + lagFactor * dt * (desiredRad - currentRadius);
                if (newRadius < kMinDist) {
                    // It's a hit! Clear approaching planet, but leave orbiting planet alone.
                    newRadius = kMinDist;
                    auto const onHitComp = dashTargetPlanet->FindComponentOfType<EventsOnHitComponent>().lock();
                    if (onHitComp != nullptr) {
                        onHitComp->OnHit(_myId);
                    }
                    gDashTargetPlanetId = EntityId();
                }
            }            

            // Now decide new angle.
            // These 2D angles are defined about the y-axis, where angle=0 is the +x axis and angle=pi/2 is the -z axis.
            float currentAngle = XZToAngle(planetToPlayerDirLocal._x, planetToPlayerDirLocal._z);
            float newAngle = currentAngle + kOrbitAngularSpeed * dt;
            Vec3 newPlanetToPlayerDir;
            AngleToXZ(newAngle, newPlanetToPlayerDir._x, newPlanetToPlayerDir._z);

            Vec3 newLocalPos = newPlanetToPlayerDir * newRadius;

            // Record for velocity calc
            Vec3 prevWorldPos = playerTrans->GetWorldPos();
            
            playerTrans->SetLocalPos(newLocalPos);

            Vec3 newWorldPos = playerTrans->GetWorldPos();

            gLastVelocity = (newWorldPos - prevWorldPos) / dt;
            gLastWorldPos = newWorldPos;
        } else {
            // Not parented to any planet. Just First-order-lag straight to planet.
            float lagFactor = 0.3f * 60.f;
            Vec3 const playerPos = playerTrans->GetWorldPos();
            Vec3 const planetPos = planetTrans->GetWorldPos();       
            Vec3 playerToPlanet = planetPos - playerPos;
            Vec3 newPos = playerPos + lagFactor * dt * playerToPlanet;
            Vec3 planetToNewPos = newPos - planetPos;
            float newDist = planetToNewPos.Normalize();
            if (newDist < kMinDist) {
                // It's a hit!
                newPos = planetPos + planetToNewPos * kMinDist;
                gDashTargetPlanetId = EntityId();
                auto const onHitComp = dashTargetPlanet->FindComponentOfType<EventsOnHitComponent>().lock();
                if (onHitComp != nullptr) {
                    onHitComp->OnHit(_myId);
                }
                // HACK: giving player a velocity so he can bounce away even if planet dies
                gLastVelocity = -playerToPlanet.GetNormalized() * 30.f;
            } else {
                gLastVelocity = (newPos - playerPos) / dt;
            }    
            playerTrans->SetWorldPos(newPos);
            gLastWorldPos = newPos;
        }
    } else {
        // Not dashing.
        if (planetWeOrbit != nullptr) {
            std::shared_ptr<TransformComponent const> planetTrans = planetWeOrbitEntity->FindComponentOfType<TransformComponent>().lock();
            assert(planetTrans != nullptr);                        

            // We parent to the planet we orbit whenever we are near the orbit radius.
            if (playerTrans->_parent.expired()) {
                Vec3 const playerPos = playerTrans->GetWorldPos();
                Vec3 const planetPos = planetTrans->GetWorldPos();

                Vec3 const playerToPlanet = planetPos - playerPos;
                float playerPlanetDist2 = playerToPlanet.Length2();
                float constexpr kParentDist = kOrbitRange * 1.5f;
                if (playerPlanetDist2 < kParentDist * kParentDist) {
                    playerTrans->Parent(planetTrans);
                    // When we parent to the planet, we set the camera to parent the planet instead of the player.
                    if (std::shared_ptr<CameraControllerComponent> camera = _camera.lock()) {
                        camera->SetTarget(planetTrans);
                    }
                }
            }

            if (std::shared_ptr<TransformComponent const> parent = playerTrans->_parent.lock()) {
                assert(parent == planetTrans);

                //
                // Radius-based fade to desired range.
                //

                // Decide new radius
                Vec3 planetToPlayerDirLocal = playerTrans->GetLocalPos();
                float currentRadius = planetToPlayerDirLocal.Normalize();
                float lagFactor = 0.3f * 60.f;                
                // 1st order lag to desired radius.
                float newRadius = currentRadius + lagFactor * dt * (kDesiredRange - currentRadius);

                // Now decide new angle.
                // These 2D angles are defined about the y-axis, where angle=0 is the +x axis and angle=pi/2 is the -z axis.
                float currentAngle = XZToAngle(planetToPlayerDirLocal._x, planetToPlayerDirLocal._z);
                float newAngle = currentAngle + kOrbitAngularSpeed * dt;
                Vec3 newPlanetToPlayerDir;
                AngleToXZ(newAngle, newPlanetToPlayerDir._x, newPlanetToPlayerDir._z);

                Vec3 newLocalPos = newPlanetToPlayerDir * newRadius;

                // Record for velocity calc
                Vec3 prevWorldPos = playerTrans->GetWorldPos();
                
                playerTrans->SetLocalPos(newLocalPos);

                Vec3 newWorldPos = playerTrans->GetWorldPos();

                gLastVelocity = (newWorldPos - prevWorldPos) / dt;
                gLastWorldPos = newWorldPos;

            } else {
                // Not yet parented; just do first-order lag to planet position.
                float lagFactor = 0.3f * 60.f;
                Vec3 const playerPos = playerTrans->GetWorldPos();
                Vec3 const planetPos = planetTrans->GetWorldPos();       
                Vec3 playerToPlanet = planetPos - playerPos;
                Vec3 newPos = playerPos + lagFactor * dt * playerToPlanet;
                playerTrans->SetWorldPos(newPos);
                gLastWorldPos = newPos;
                gLastVelocity = (newPos - playerPos) / dt;
            }
        } else {
            // Just fade current velocity to 0.        
            float lagFactor = 0.2f * 60.f;
            gLastVelocity -= lagFactor * dt * gLastVelocity;

            auto playerTrans = _transform.lock();
            Vec3 newPos = playerTrans->GetWorldPos() + gLastVelocity * dt;
            playerTrans->SetWorldPos(newPos);
            gLastWorldPos = newPos;
        }
    }

    return false;
}

EntityId PlayerOrbitControllerComponent::GetNextPlanetFromInput(Vec3 const& inputVec) const {
    Vec3 playerPos = _transform.lock()->GetWorldPos();
    if (inputVec.IsZero()) {
        // Don't change planets
        return _planetWeOrbitId;    
    }

    std::shared_ptr<TransformComponent> currentPlanetTrans;
    Entity* currentPlanetEntity = _entityMgr->GetEntity(_planetWeOrbitId);
    if (currentPlanetEntity != nullptr) {
        currentPlanetTrans = currentPlanetEntity->FindComponentOfType<TransformComponent>().lock();
    }     

    // WE USE CURRENTLY ORBITED PLANET AS REFERENCE, NOT PLAYER
    float constexpr kMinDotWithInput = 0.707106781f;
    float closestDist = std::numeric_limits<float>::max();    
    EntityId potentialNewPlanetId = _planetWeOrbitId;
    if (currentPlanetTrans != nullptr) {
        playerPos = currentPlanetTrans->GetWorldPos();
    }
    _entityMgr->ForEveryActiveEntity([&](EntityId id) {
        if (id == _planetWeOrbitId) {
            return;
        }
        Entity& e = *_entityMgr->GetEntity(id);
        std::shared_ptr<OrbitableComponent> planet = e.FindComponentOfType<OrbitableComponent>().lock();
        if (planet == nullptr) {
            return;
        }
        // if (planet == currentPlanet) {
        //     return;
        // }
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
            potentialNewPlanetId = id;
        }
    });

    return potentialNewPlanetId;
}