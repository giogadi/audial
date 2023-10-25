#include "flow_player.h"

#include "game_manager.h"
#include "typing_enemy.h"
#include "input_manager.h"
#include "renderer.h"
#include "camera.h"
#include "math_util.h"
#include "beat_clock.h"
#include "geometry.h"

#include "entities/flow_pickup.h"
#include "entities/flow_wall.h"
#include "entities/vfx.h"

void FlowPlayerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutFloat("launch_vel", _p._defaultLaunchVel);
    pt.PutFloat("max_horiz_speed_after_dash", _p._maxHorizSpeedAfterDash);
    pt.PutFloat("max_vert_speed_after_dash", _p._maxVertSpeedAfterDash);
    pt.PutFloat("max_overall_speed_after_dash", _p._maxOverallSpeedAfterDash);
    pt.PutFloat("dash_time", _p._dashTime);
    serial::SaveInNewChildOf(pt, "gravity", _p._gravity);
    pt.PutFloat("max_fall_speed", _p._maxFallSpeed);
}

void FlowPlayerEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    _p._defaultLaunchVel = pt.GetFloat("launch_vel");
    pt.TryGetFloat("max_horiz_speed_after_dash", &_p._maxHorizSpeedAfterDash);
    pt.TryGetFloat("max_vert_speed_after_dash", &_p._maxVertSpeedAfterDash);
    pt.TryGetFloat("max_overall_speed_after_dash", &_p._maxOverallSpeedAfterDash);
    pt.TryGetFloat("dash_time", &_p._dashTime);
    serial::LoadFromChildOf(pt, "gravity", _p._gravity);
    pt.TryGetFloat("max_fall_speed", &_p._maxFallSpeed);
}

void FlowPlayerEntity::InitDerived(GameManager& g) {
    _s = State();
    {
        ne::Entity* camera = g._neEntityManager->GetFirstEntityOfType(ne::EntityType::Camera);
        if (camera == nullptr) {
            printf("FlowPlayerEntity::InitDerived: couldn't find a camera entity!\n");
        } else {
            _s._cameraId = camera->_id;
        }        
    }
    _s._currentColor = _modelColor;
    _s._respawnPos = _initTransform.Pos();
    _transform.SetPos(_s._respawnPos);
}

namespace {

FlowWallEntity* FindOverlapWithWall(GameManager& g, Transform const& playerTrans, Vec3* penetrationOut) {
    ne::EntityManager::Iterator wallIter = g._neEntityManager->GetIterator(ne::EntityType::FlowWall);
    std::vector<Vec3> aabbPoly(4);
    for (; !wallIter.Finished(); wallIter.Next()) {
        FlowWallEntity* pWall = static_cast<FlowWallEntity*>(wallIter.GetEntity());
        if (!pWall->_canHit) {
            continue;
        }
        Transform const& wallTrans = pWall->_transform;
        Vec3 euler = wallTrans.Quat().EulerAngles();
        float yRot = -euler._y;
        Vec3 penetration;
        bool hasOverlap = false;
        if (pWall->_polygon.size() < 3) {
            Vec3 halfScale = wallTrans.Scale() * 0.5f + 0.5f * playerTrans.Scale();
            aabbPoly[0].Set(halfScale._x, 0.f, halfScale._z);
            aabbPoly[1].Set(halfScale._x, 0.f, -halfScale._z);
            aabbPoly[2].Set(-halfScale._x, 0.f, -halfScale._z);
            aabbPoly[3].Set(-halfScale._x, 0.f, halfScale._z);
            
            hasOverlap = geometry::PointInConvexPolygon2D(playerTrans.Pos(), aabbPoly, wallTrans.Pos(), yRot, Vec3(1.f, 1.f, 1.f), &penetration);
        } else {
            hasOverlap = geometry::PointInConvexPolygon2D(playerTrans.Pos(), pWall->_polygon, wallTrans.Pos(), yRot, wallTrans.Scale() + (playerTrans.Scale() /** 0.5f*/), &penetration);
        }
        if (hasOverlap) {
            *penetrationOut = penetration;
            return pWall;
        }
    }
    return nullptr;
}

}  // namespace

void FlowPlayerEntity::Draw(GameManager& g, float const dt) {
    // Draw history positions
    Mat4 historyTrans;
    historyTrans.Scale(0.1f, 0.1f, 0.1f);
    for (Vec3 const& prevPos : _s._posHistory) {
        historyTrans.SetTranslation(prevPos);
        renderer::ModelInstance& model = g._scene->DrawCube(historyTrans, _s._currentColor);
         model._topLayer = true;
    }

    if (_model != nullptr) {
        renderer::ModelInstance& model = g._scene->DrawMesh(_model, _transform.Mat4Scale(), _s._currentColor);
         model._topLayer = true;
    }

    if (_s._dashTimer >= 0.f && (_s._useLastKnownDashTarget || _s._isPushDash)) {
        g._scene->DrawLine(_transform.Pos(), _s._lastKnownDashTarget, _s._lastDashTargetColor);
    }
}

void FlowPlayerEntity::Respawn(GameManager& g) {
    _transform.SetTranslation(_s._respawnPos);
    _s._posHistory.clear();
    _s._framesSinceLastSample = 0;
    _s._vel.Set(0.f,0.f,0.f);
    _s._dashTimer = -1.f;
    _s._respawnBeforeFirstDash = true;

    // Reset everything from the current section
    for (ne::EntityManager::AllIterator iter = g._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity* e = iter.GetEntity();
        if (e->_flowSectionId >= 0 && e->_flowSectionId == _s._currentSectionId) {
            e->Init(g);
        }
    }
    // Reactivate all inactive entities that were initially active    
    for (ne::EntityManager::AllIterator iter = g._neEntityManager->GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity* e = iter.GetEntity();
        if (e->_initActive && e->_flowSectionId >= 0 && e->_flowSectionId == _s._currentSectionId) {
            g._neEntityManager->TagForActivate(e->_id, /*initOnActivate=*/true);
        }
    }
    // deactivate all initially-inactive things
    /*for (ne::EntityManager::AllIterator iter = g._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity* e = iter.GetEntity();
        if (!e->_initActive && e->_flowSectionId >= 0 && e->_flowSectionId == _currentSectionId) {
            g._neEntityManager->TagForDeactivate(e->_id);
        }
    }*/

    // Activate the on-respawn entity
    if (ne::Entity* e = g._neEntityManager->GetEntity(_s._toActivateOnRespawn, /*includeActive=*/false, /*includeInactive=*/true)) {
        g._neEntityManager->TagForActivate(e->_id, /*initOnActivate=*/true);
    }
    else if (ne::Entity* e = g._neEntityManager->GetEntity(_s._toActivateOnRespawn)) {
        e->Init(g);
    }
}

void FlowPlayerEntity::SetNewSection(GameManager& g, int newSectionId) {
    _s._currentSectionId = newSectionId;
}

void FlowPlayerEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (_s._countOffEndTime >= 0.0 && beatTime < _s._countOffEndTime) {
        double unusedBeatNum;
        float beatTimeFrac = (float) (modf(beatTime, &unusedBeatNum));
        // 0 is default color, 1 is temp flash color
        float factor = math_util::SmoothStep(beatTimeFrac);
        Vec4 constexpr kFlashColor(0.f, 0.f, 0.f, 1.f);
        _s._currentColor = _modelColor + factor * (kFlashColor - _modelColor);
        return;
    } else {
        _s._currentColor = _modelColor;
    }

    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    Vec3 const& playerPos = _transform.GetPos();    
    Mat4 viewProjTransform = g._scene->GetViewProjTransform();
    bool const usingController = g._inputManager->IsUsingController();
    for (ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !enemyIter.Finished(); enemyIter.Next()) {
        // Vec4 constexpr kGreyColor(0.6f, 0.6f, 0.6f, 0.7f);
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
        Vec3 dp = playerPos - enemy->_transform.GetPos();
        dp._y = 0.f;
        float d2 = dp.Length2();
        if (!enemy->CanHit(*this, g)) {
            continue;
        }
        
        // bool clear = IsCollisionFree(g, playerPos, enemy->_transform.GetPos());
        // if (!clear) {
        //     enemy->_currentColor = kGreyColor;
        //     enemy->_textColor = kGreyColor;
        //     continue;
        // }
        // enemy->_s._currentColor = enemy->_modelColor;

        if (usingController) {
            InputManager::ControllerButton nextButton = enemy->GetNextButton();
            if (!g._inputManager->IsKeyPressedThisFrame(nextButton)) {
                continue;
            }
        } else {
            InputManager::Key nextKey = enemy->GetNextKey();
            if (!g._inputManager->IsKeyPressedThisFrame(nextKey)) {
                continue;
            }
        }        

        if (nearest == nullptr || d2 < nearestDist2) {

            // Check if it's in view of camera
            float screenX, screenY;
            geometry::ProjectWorldPointToScreenSpace(enemy->_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);
            if (screenX < 0 || screenX > g._windowWidth || screenY < 0 || screenY > g._windowHeight) {
                continue;
            }
            
            nearest = enemy;
            nearestDist2 = d2;
        }
    }

    if (nearest != nullptr) {
        _s._respawnBeforeFirstDash = false;
        nearest->OnHit(g);
        Vec3 toEnemyDir;
        if (nearest->_p._pushAngleDeg >= 0.f) {
            float angleRad = nearest->_p._pushAngleDeg * kDeg2Rad;
            toEnemyDir._x = -cos(angleRad);
            toEnemyDir._z = sin(angleRad);
        } else {
            toEnemyDir = nearest->_transform.GetPos() - playerPos;
        }
        toEnemyDir._y = 0.f;
        toEnemyDir.Normalize();
        _s._isPushDash = nearest->_p._enemyType == TypingEnemyType::Push;
        float sign = _s._isPushDash ? -1.f : 1.f;
        float launchVel = (nearest->_p._dashVelocity >= 0.f) ? nearest->_p._dashVelocity : _p._defaultLaunchVel;
        _s._vel = toEnemyDir * (sign * launchVel);
        _s._dashTimer = 0.f;
        _s._useLastKnownDashTarget = sign > 0.f;
        _s._dashTargetId = nearest->_id;
        _s._lastKnownDashTarget = nearest->_transform.Pos();
        _s._lastKnownDashTarget._y = _transform.Pos()._y;
        _s._lastDashTargetColor = nearest->_modelColor;
        _s._lastDashTargetColor._w = 1.f;
        _s._applyGravityDuringDash = false;
        _s._stopDashOnPassEnemy = nearest->_p._stopOnPass;

        ne::EntityId nearestId = nearest->_id;
        for (auto enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !enemyIter.Finished(); enemyIter.Next()) {
            TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
            if (enemy->_id != nearestId) {
                enemy->OnHitOther(g);
            }
        }
    }

    // Check if we're done dashing
    {
        bool doneDashing = false;
        bool passedDashTarget = false;        
        if (_s._dashTimer >= _p._dashTime) {
            doneDashing = true;
        } else if (_s._dashTimer >= 0.f) {
            Vec3 playerPos = _transform.Pos();
            TypingEnemyEntity* dashTarget = static_cast<TypingEnemyEntity*>(g._neEntityManager->GetEntity(_s._dashTargetId));
            if (dashTarget != nullptr) {
                _s._lastKnownDashTarget = dashTarget->_transform.Pos();
                _s._lastKnownDashTarget._y = playerPos._y;
            }
            if (_s._useLastKnownDashTarget) {
                Vec3 playerToTargetDir = _s._lastKnownDashTarget - playerPos;
                float dotP = Vec3::Dot(playerToTargetDir, _s._vel);
                if (dotP < 0.f) {
                    // TODO: do we even need this case? We're already "looking
                    // ahead" for crossings in the dotPAhead case below.
                    passedDashTarget = true;
                    _s._useLastKnownDashTarget = false;
                    _transform.SetPos(_s._lastKnownDashTarget);
                } else {
                    float currentSpeed = _s._vel.Length();
                    Vec3 newVel = playerToTargetDir.GetNormalized() * currentSpeed;
                    _s._vel = newVel;
                    Vec3 newPos = playerPos + newVel * dt;
                    float dotPAgain = Vec3::Dot(newPos - _s._lastKnownDashTarget, playerPos - _s._lastKnownDashTarget);
                    if (dotPAgain < 0.f) {
                        passedDashTarget = true;
                        _s._useLastKnownDashTarget = false;
                        _transform.SetPos(_s._lastKnownDashTarget);
                    } else {
                        _s._vel = newVel;
                    }
                }
            }
        }

        if (passedDashTarget && _s._stopDashOnPassEnemy) {
            // constexpr float kPassTargetSpeed = 1.f;
            constexpr float kPassTargetSpeed = 0.f;
            float speed = _s._vel.Normalize();
            speed = std::min(speed, kPassTargetSpeed);
            _s._vel *= speed;
        }

        if (doneDashing) {
            if (passedDashTarget) {
                constexpr float kPassTargetSpeed = 0.f;
                float speed = _s._vel.Normalize();
                speed = std::min(speed, kPassTargetSpeed);
                _s._vel *= speed;
            } else {
                if (_p._maxHorizSpeedAfterDash >= 0.f) {
                    // TODO: make "maxSpeedAfterPush" a separate property.
                    float const maxSpeed = _s._isPushDash ? 2.f : _p._maxHorizSpeedAfterDash;
                    _s._vel._x = math_util::Clamp(_s._vel._x, -maxSpeed, maxSpeed);
                }
                if (_p._maxVertSpeedAfterDash >= 0.f) {
                    _s._vel._z = math_util::Clamp(_s._vel._z, -_p._maxVertSpeedAfterDash, _p._maxVertSpeedAfterDash);
                }
                if (_p._maxOverallSpeedAfterDash >= 0.f) {
                    float speed = _s._vel.Normalize();
                    speed = std::min(_p._maxOverallSpeedAfterDash, speed);
                    _s._vel *= speed;
                }
            }
            _s._dashTimer = -1.f;
            _s._dashTargetId = ne::EntityId();
        }
    }
    
    // Update state based on dash status
    if (_s._dashTimer >= 0.f) {
        if (_s._applyGravityDuringDash) {
            _s._vel += _p._gravity * dt;
        }

        // Still dashing. maintain velocity.
        _s._dashTimer += dt;
    } else {
        // Not dashing. Apply gravity if we've dashed since respawn.
        if (!_s._respawnBeforeFirstDash) {
            _s._vel += _p._gravity * dt;
            if (_p._maxFallSpeed >= 0.f) {
                _s._vel._z = std::min(_p._maxFallSpeed, _s._vel._z);
            }            
        }
    }

    Vec3 p = _transform.GetPos();

    // Update prev positions
    int constexpr kMaxHistory = 5;
    if (_s._posHistory.size() == kMaxHistory) {
        _s._posHistory.pop_back();
    }

    ++_s._framesSinceLastSample;
    int constexpr kFramesPerSample = 2;
    if (_s._framesSinceLastSample >= kFramesPerSample) {
        _s._posHistory.push_front(p);
        _s._framesSinceLastSample = 0;
    }

    p += _s._vel * dt;
    _transform.SetTranslation(p);

    // Check collisions between p and walls    
    Vec3 penetration;
    FlowWallEntity* pHitWall = FindOverlapWithWall(g, _transform, &penetration);
    penetration._y = 0.f;
    if (pHitWall != nullptr) {
        // Respawn(g);
        pHitWall->OnHit(g, -penetration.GetNormalized());
        p += penetration * 1.1f;  // Add some padding to ensure we're outta collision
        Vec3 collNormal = penetration.GetNormalized();
        Vec3 newVel = -_s._vel;
        // reflect across coll normal
        Vec3 tangentPart = Vec3::Dot(newVel, collNormal) * collNormal;
        Vec3 normalPart = newVel - tangentPart;
        newVel -= 2 * normalPart;        

        // after bounce, ensure we have some velocity in the collision normal
        // float constexpr kMinBounceSpeed = 20.f;
        // float speedAlongNormal = Vec3::Dot(newVel, collNormal);
        // if (speedAlongNormal < kMinBounceSpeed) {
        //     Vec3 correction = (kMinBounceSpeed - speedAlongNormal) * collNormal;
        //     newVel += correction;
        // }
        float constexpr kBounceSpeed = 5.f;
        newVel = tangentPart.GetNormalized() * kBounceSpeed;

        _transform.SetTranslation(p);
        _s._vel = newVel;

        // Pretend it's a new dash with gravity applied
        _s._dashTimer = 0.25f * _p._dashTime;
        _s._applyGravityDuringDash = false;
        _s._dashTargetId = ne::EntityId();
        _s._useLastKnownDashTarget = false;
        _s._isPushDash = false;
    }

    // Check if we hit any pickups
    ne::EntityManager::Iterator pickupIter = g._neEntityManager->GetIterator(ne::EntityType::FlowPickup);
    for (; !pickupIter.Finished(); pickupIter.Next()) {
        FlowPickupEntity* pPickup = static_cast<FlowPickupEntity*>(pickupIter.GetEntity());
        assert(pPickup != nullptr);
        Transform const& pickupTrans = pPickup->_transform;
        Vec3 penetration;
        bool hasOverlap = geometry::DoAABBsOverlap(_transform, pickupTrans, &penetration);
        if (hasOverlap) {
            pPickup->OnHit(g);
        }
    }

    bool respawned = false;
    if (_s._killMaxZ.has_value()) {
        Vec3 p = _transform.Pos();
        if (p._z > _s._killMaxZ.value()) {
            Respawn(g);
            respawned = true;
        }
    }
    if (!respawned && (_s._killIfBelowCameraView || _s._killIfLeftOfCameraView)) {
        Vec3 p = _transform.Pos();
        float playerSize = _transform.Scale()._x;
        float screenX, screenY;
        geometry::ProjectWorldPointToScreenSpace(p, viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);
        if (_s._killIfBelowCameraView && screenY > g._windowHeight + 4 * playerSize) {
            Respawn(g);
            respawned = true;
        } else if (_s._killIfLeftOfCameraView && screenX < -4 * playerSize) {
            Respawn(g);
            respawned = true;
        }
    }
}
