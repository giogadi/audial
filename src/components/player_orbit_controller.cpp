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
    float constexpr kDecel = 25.f;
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

static bool gApproaching = false;
static bool gAssociatedWithAPlanet = false;
static Vec3 gLastVelocity;
static Vec3 gLastWorldPos;

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
            gAssociatedWithAPlanet = true;
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
                    // It's a hit!
                    newRadius = kMinDist;
                    gApproaching = false;
                    Entity* planetEntity = _entityMgr->GetEntity(_planetWeOrbitId);
                    assert(planetEntity != nullptr);
                    auto const comp = planetEntity->FindComponentOfType<EventsOnHitComponent>().lock();
                    if (comp != nullptr) {
                        comp->OnHit(_myId);
                    }
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

            // Record for velocity calc
            Vec3 prevWorldPos = playerTrans->GetWorldPos();
            
            playerTrans->SetLocalPos(newLocalPos);

            Vec3 newWorldPos = playerTrans->GetWorldPos();

            gLastVelocity = (newWorldPos - prevWorldPos) / dt;
            gLastWorldPos = newWorldPos;
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
                // It's a hit!
                newPos = planetPos + planetToNewPos * kMinDist;
                playerTrans->Parent(planetTrans);
                gApproaching = false;
                // When we parent to a new planet, the camera controller should follow the planet.
                if (std::shared_ptr<CameraControllerComponent> camera = _camera.lock()) {
                    camera->SetTarget(planetTrans);
                }
                Entity* planetEntity = _entityMgr->GetEntity(_planetWeOrbitId);
                assert(planetEntity != nullptr);
                auto const comp = planetEntity->FindComponentOfType<EventsOnHitComponent>().lock();
                if (comp != nullptr) {
                    comp->OnHit(_myId);
                }
                // HACK: giving player a velocity so he can bounce away even if planet dies
                gLastVelocity = -playerToPlanet.GetNormalized() * 30.f;
            } else {
                gLastVelocity = (newPos - playerPos) / dt;
            }            
            playerTrans->SetWorldPos(newPos);
            gLastWorldPos = newPos;
        }
    }

    // If the planet disappeared while we were tracking it, decel to a stop and have the camera track the player.
    if (gAssociatedWithAPlanet && _planetWeOrbit.expired()) {
        gAssociatedWithAPlanet = false;
        std::shared_ptr<TransformComponent> playerTrans = _transform.lock();        
        if (std::shared_ptr<CameraControllerComponent> camera = _camera.lock()) {
            camera->SetTarget(playerTrans);
        }
        // In case we were parented to the destroyed planet, reset the planet's world position.
        assert(playerTrans->_parent.expired());
        playerTrans->SetWorldPos(gLastWorldPos);        
    }

    if (!gAssociatedWithAPlanet) {
        // Get new position by applying first-order-lag to last velocity toward 0.
        float lagFactor = 0.2f;
        gLastVelocity -= lagFactor * gLastVelocity;

        auto playerTrans = _transform.lock();
        Vec3 newPos = playerTrans->GetWorldPos() + gLastVelocity * dt;
        playerTrans->SetWorldPos(newPos);
        gLastWorldPos = newPos;
    }

    // HOWDY DEBUG
    // Vec3 playerWorldPos = _transform.lock()->GetWorldPos();
    // gLastWorldPos = playerWorldPos;

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
    EntityId potentialNewPlanetId;
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
            potentialNewPlanetId = id;
            dashDir = (planetPos - _transform.lock()->GetWorldPos()).GetNormalized();
        }
    });

    if (currentPlanet) {
        currentPlanet->OnLeaveOrbit(*_g);
    }
    if (potentialNewPlanet) {
        _planetWeOrbit = potentialNewPlanet;
        _planetWeOrbitId = potentialNewPlanetId;
    }    

    return true;
}