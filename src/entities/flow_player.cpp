#include "flow_player.h"

#include "game_manager.h"
#include "typing_enemy.h"
#include "input_manager.h"
#include "renderer.h"
#include "camera.h"
#include "math_util.h"
#include "beat_clock.h"
#include "geometry.h"
#include "seq_action.h"

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

    Transform renderTrans = _transform;

    if (_s._moveState == MoveState::Pull || _s._moveState == MoveState::Push) {

        g._scene->DrawLine(_transform.Pos(), _s._lastKnownDashTarget, _s._lastKnownDashTargetColor);

        if (_s._vel.Length2() > 0.1f) {
            _s._dashAnimDir = _s._vel.GetNormalized();
        }
    }

    float constexpr kMaxSquishInTime = 0.1f;
    float constexpr kMaxSquishOutTime = 0.1f;    
    switch (_s._dashAnimState) {
    case DashAnimState::Accel: {
        float const kSquishDelta = dt / kMaxSquishInTime;
        _s._dashSquishFactor += kSquishDelta;
        if (_s._dashSquishFactor > 1.f) {
            _s._dashSquishFactor = 1.f;
            _s._dashAnimState = DashAnimState::Decel;
        }
    }
        break;
    case DashAnimState::Decel: {
        float const kSquishDelta = dt / kMaxSquishOutTime;
        _s._dashSquishFactor -= kSquishDelta;
        if (_s._dashSquishFactor < 0.f) {
            _s._dashSquishFactor = 0.f;
            _s._dashAnimState = DashAnimState::None;
        }
    }
        break;
    case DashAnimState::None:
        _s._dashSquishFactor = 0.f;
        break;
    }

    if (_s._dashSquishFactor > 0.f) {
        Vec3 newX = _s._dashAnimDir;
        Vec3 newY(0.f, 1.f, 0.f);
        Mat3 rotMat;
        rotMat.SetCol(0, newX);
        rotMat.SetCol(1, newY);
        Vec3 newZ = Vec3::Cross(newX, newY);
        rotMat.SetCol(2, newZ);
        Quaternion newQ;
        newQ.SetFromRotMat(rotMat);
        renderTrans.SetQuat(newQ);

        float constexpr kTangentSquishAtMaxSpeed = 5.f;
        float constexpr kOrthoSquishAtMinSpeed = 0.5f;
        float constexpr kOrthoSquishAtMaxSpeed = 0.5f;
        float constexpr kSpeedOfMaxSquish = 40.f;
        float speedFactor = _s._dashLaunchSpeed / kSpeedOfMaxSquish;
        float const kMaxTangentSquish = 1.f + speedFactor * (kTangentSquishAtMaxSpeed - 1.f);
        float const kMaxOrthoSquish = kOrthoSquishAtMinSpeed + speedFactor * (kOrthoSquishAtMaxSpeed - kOrthoSquishAtMinSpeed);
        float squishFactor = math_util::Clamp(_s._dashSquishFactor, 0.f, 1.f);
        float tangentSquish = 1.f + squishFactor * (kMaxTangentSquish - 1.f);
        float orthoSquish = 1.f + squishFactor * (kMaxOrthoSquish - 1.f);
        Vec3 squishScale(tangentSquish, orthoSquish, orthoSquish);
        renderTrans.ApplyScale(squishScale);
        float scaleDiffInMotionDir = renderTrans.Scale()._x - _transform.Scale()._x;
        Vec3 p = renderTrans.Pos();
        p -= _s._dashAnimDir * (0.5f * scaleDiffInMotionDir);
        renderTrans.SetPos(p);
    }

    if (_model != nullptr) {
        renderer::ModelInstance& model = g._scene->DrawMesh(_model, renderTrans.Mat4Scale(), _s._currentColor);
        model._topLayer = true;
    }
}

void FlowPlayerEntity::Respawn(GameManager& g) {
    _transform.SetTranslation(_s._respawnPos);
    _s._posHistory.clear();
    _s._framesSinceLastSample = 0;
    _s._vel.Set(0.f,0.f,0.f);
    _s._dashTimer = -1.f;
    _s._respawnBeforeFirstInteract = true;

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

namespace {
    bool InteractingWithEnemy(FlowPlayerEntity::MoveState s) {
        switch (s) {
        case FlowPlayerEntity::MoveState::Default: return false;
        case FlowPlayerEntity::MoveState::WallBounce: return false;
        case FlowPlayerEntity::MoveState::Pull: return true;
        case FlowPlayerEntity::MoveState::PullStop: return true;
        case FlowPlayerEntity::MoveState::Push: return true;
        }
        return false;
    }
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
    InputManager::Key hitKey = InputManager::Key::NumKeys;
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
            if (nextButton == InputManager::ControllerButton::Count || !g._inputManager->IsKeyPressedThisFrame(nextButton)) {
                continue;
            }
        } else {
            InputManager::Key nextKey = enemy->GetNextKey();
            if (nextKey == InputManager::Key::NumKeys || !g._inputManager->IsKeyPressedThisFrame(nextKey)) {
                continue;
            }
            hitKey = nextKey;
        }

        if (enemy->_id == _s._lastHitEnemy) {
            // Check if it's in view of camera
            float screenX, screenY;
            geometry::ProjectWorldPointToScreenSpace(enemy->_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);
            if (screenX < 0 || screenX > g._windowWidth || screenY < 0 || screenY > g._windowHeight) {
                continue;
            }
            nearest = enemy;
            nearestDist2 = d2;
            break;
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

    //
    // Check if we start a new enemy interaction
    // 
    if (nearest != nullptr) {
        _s._respawnBeforeFirstInteract = false;
        
        HitResult hitResult = nearest->OnHit(g);
        HitResponse const& hitResponse = hitResult._response;

        _s._lastHitEnemy = nearest->_id;
        _s._heldKey = hitKey;
        
        switch (hitResponse._type) {
            case HitResponseType::None:
                break;
            case HitResponseType::Pull: {
                _s._interactingEnemyId = nearest->_id;
                _s._stopAfterTimer = hitResponse._stopAfterTimer;
                _s._stopDashOnPassEnemy = hitResponse._stopOnPass;
                // If we are close enough to the enemy, just stick to them and start in PullStop if applicable.
                if (_s._stopDashOnPassEnemy && nearestDist2 < 0.001f * 0.001f) {
                    _transform.SetPos(nearest->_transform.Pos());
                    _s._moveState = MoveState::PullStop;
                    _s._passedPullTarget = true;
                    _s._dashAnimState = DashAnimState::None;
                } else {
                    _s._moveState = MoveState::Pull;
                    _s._passedPullTarget = false;
                    _s._dashAnimState = DashAnimState::Accel;
                }
                _s._dashTimer = 0.f;                        
                if (hitResponse._speed >= 0.f) {
                    _s._dashLaunchSpeed = hitResponse._speed;
                } else {
                    _s._dashLaunchSpeed = _p._defaultLaunchVel;
                }
                Vec3 playerToTargetDir = nearest->_transform.Pos() - playerPos;
                float dist = playerToTargetDir.Normalize();
                _s._vel = playerToTargetDir * _s._dashLaunchSpeed;
                if (dist < 0.001f) {
                    playerToTargetDir.Set(1.f, 0.f, 0.f);
                }
                nearest->Bump(playerToTargetDir);
            }
                break;
            case HitResponseType::Push: {
                _s._interactingEnemyId = nearest->_id;
                _s._moveState = MoveState::Push;
                _s._stopAfterTimer = hitResponse._stopAfterTimer;
                _s._dashTimer = 0.f;
                _s._dashAnimState = DashAnimState::Accel;
                Vec3 pushDir;
                if (hitResponse._pushAngleDeg >= 0.f) {
                    float angleRad = hitResponse._pushAngleDeg * kDeg2Rad;
                    pushDir._x = cos(angleRad);
                    pushDir._y = 0.f;
                    pushDir._z = -sin(angleRad);
                } else {
                    pushDir = playerPos - nearest->_transform.GetPos();
                    pushDir._y = 0.f;
                    pushDir.Normalize();
                }
                if (hitResponse._speed >= 0.f) {
                    _s._dashLaunchSpeed = hitResponse._speed;
                } else {
                    _s._dashLaunchSpeed = _p._defaultLaunchVel;
                }
                _s._vel = pushDir * _s._dashLaunchSpeed;
                nearest->Bump(pushDir);
            }
                break;
            case HitResponseType::Count: {
                assert(false);
                break;
            }
        }

        ne::EntityId nearestId = nearest->_id;
        for (auto enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !enemyIter.Finished(); enemyIter.Next()) {
            TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
            if (enemy->_id != nearestId) {
                enemy->OnHitOther(g);
            }
        }
    }

    //
    // Check if we're done with an existing interaction
    //
    switch (_s._moveState) {
        case MoveState::Default: break;
        case MoveState::Pull: {
            bool finishedPulling = false;
            if (_s._stopAfterTimer && _s._dashTimer >= _p._dashTime) {
                finishedPulling = true;
            }
            if (finishedPulling) {
                _s._moveState = MoveState::Default;
                _s._interactingEnemyId = ne::EntityId();
                if (_p._maxHorizSpeedAfterDash >= 0.f) {
                    _s._vel._x = math_util::ClampAbs(_s._vel._x, _p._maxHorizSpeedAfterDash);
                }
                if (_p._maxVertSpeedAfterDash >= 0.f) {
                    _s._vel._z = math_util::ClampAbs(_s._vel._z, _p._maxVertSpeedAfterDash);
                }
                if (_p._maxOverallSpeedAfterDash >= 0.f) {
                    float speed = _s._vel.Normalize();
                    speed = std::min(_p._maxOverallSpeedAfterDash, speed);
                    _s._vel *= speed;
                }
            }
        }
        break;

        case MoveState::PullStop: {
            float constexpr kStopTime = 0.5f;
            if (_s._dashTimer >= kStopTime) {
                _s._moveState = MoveState::Default;
                if (TypingEnemyEntity* e = g._neEntityManager->GetEntityAs<TypingEnemyEntity>(_s._interactingEnemyId)) {
                    e->DoComboEndActions(g);
                }
                _s._interactingEnemyId = ne::EntityId();
            }
        }
        break;

        case MoveState::Push: {
            bool finishedPushing = false;
            if (_s._heldKey != InputManager::Key::NumKeys && g._inputManager->IsKeyReleasedThisFrame(_s._heldKey)) {
                finishedPushing = true;
                _s._heldKey = InputManager::Key::NumKeys;
            } else if (_s._stopAfterTimer && _s._dashTimer >= _p._dashTime) {
                finishedPushing = true;
            }
            if (finishedPushing) {
                _s._moveState = MoveState::Default;
                float maxHorizSpeed = std::max(2.f, _p._maxHorizSpeedAfterDash);
                if (_p._maxHorizSpeedAfterDash >= 0.f) {
                    _s._vel._x = math_util::ClampAbs(_s._vel._x, maxHorizSpeed);
                }
                if (_p._maxVertSpeedAfterDash >= 0.f) {
                    _s._vel._z = math_util::ClampAbs(_s._vel._z, _p._maxVertSpeedAfterDash);
                }
                if (_p._maxOverallSpeedAfterDash >= 0.f) {
                    float speed = _s._vel.Normalize();
                    speed = std::min(_p._maxOverallSpeedAfterDash, speed);
                    _s._vel *= speed;
                }
            }
        }
        break;

        case MoveState::WallBounce: {
            // TODO make this a separate property!
            float const wallBounceTime = 0.5f * _p._dashTime;
            if (_s._dashTimer >= wallBounceTime) {
                _s._moveState = MoveState::Default;
                // TODO: Should bounce transition be the same as pull's?
                if (_p._maxHorizSpeedAfterDash >= 0.f) {
                    _s._vel._x = math_util::ClampAbs(_s._vel._x, _p._maxHorizSpeedAfterDash);
                }
                if (_p._maxVertSpeedAfterDash >= 0.f) {
                    _s._vel._z = math_util::ClampAbs(_s._vel._z, _p._maxVertSpeedAfterDash);
                }
                if (_p._maxOverallSpeedAfterDash >= 0.f) {
                    float speed = _s._vel.Normalize();
                    speed = std::min(_p._maxOverallSpeedAfterDash, speed);
                    _s._vel *= speed;
                }
            }
        }
        break;
    }

    //
    // Update position/velocity/etc based on current state
    //
    Vec3 const previousPos = _transform.Pos();
    Vec3 newPos = previousPos;
    if (InteractingWithEnemy(_s._moveState)) {
        if (TypingEnemyEntity* dashTarget = g._neEntityManager->GetEntityAs<TypingEnemyEntity>(_s._interactingEnemyId)) {
            _s._lastKnownDashTarget = dashTarget->_transform.Pos();
            _s._lastKnownDashTarget._y = playerPos._y;
            _s._lastKnownDashTargetColor = dashTarget->_s._currentColor;
            _s._lastKnownDashTargetColor._w = 1.f;
        }
    }
    switch (_s._moveState) {
        case MoveState::Default: {
            if (!_s._respawnBeforeFirstInteract) {
                _s._vel += _p._gravity * dt;
                if (_p._maxFallSpeed >= 0.f) {
                    _s._vel._z = std::min(_p._maxFallSpeed, _s._vel._z);
                }
            }
            newPos += _s._vel * dt;
        }
        break;
        case MoveState::WallBounce: {
            _s._dashTimer += dt;
            newPos += _s._vel * dt;            
        }
        break;
        case MoveState::Pull: {
            _s._dashTimer += dt;
            if (_s._passedPullTarget) {
                newPos += _s._vel * dt;
            } else {
                Vec3 playerToTargetDir = _s._lastKnownDashTarget - previousPos;
                playerToTargetDir.Normalize();
                float currentSpeed = _s._vel.Length();
                _s._vel = playerToTargetDir * currentSpeed;
                newPos += _s._vel * dt;
                Vec3 newPlayerToTarget = _s._lastKnownDashTarget - newPos;
                float dotP = Vec3::Dot(newPlayerToTarget, _s._vel);
                if (dotP < 0.f) {
                    // we passed the dash target
                    _s._passedPullTarget = true;
                    if (_s._stopDashOnPassEnemy) {
                        _s._vel.Set(0.f, 0.f, 0.f);
                        newPos = _s._lastKnownDashTarget;
                        _s._moveState = MoveState::PullStop;
                        _s._dashTimer = 0.f;
                    }
                }
            }
        }
        break;
        case MoveState::PullStop: {
            _s._dashTimer += dt;
        }
        break;
        case MoveState::Push: {
            _s._dashTimer += dt;
            newPos += _s._vel * dt;
        }
        break;        
    }

    _transform.SetPos(newPos);


    // Check if we've hit a wall
    //
    // TODO: Should we run this after respawn logic?
    {
        Vec3 penetration;
        FlowWallEntity* pHitWall = FindOverlapWithWall(g, _transform, &penetration);
        penetration._y = 0.f;
        if (pHitWall != nullptr) {
            // Respawn(g);
            _s._moveState = MoveState::WallBounce;
            _s._interactingEnemyId = ne::EntityId();
            pHitWall->OnHit(g, -penetration.GetNormalized());
            newPos += penetration * 1.1f;  // Add some padding to ensure we're outta collision
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

            _transform.SetTranslation(newPos);
            _s._vel = newVel;
            _s._dashTimer = 0.f;
            _s._dashAnimState = DashAnimState::None;
        }
    }

    // Update prev positions
    int constexpr kMaxHistory = 5;
    if (_s._posHistory.size() == kMaxHistory) {
        _s._posHistory.pop_back();
    }

    ++_s._framesSinceLastSample;
    int constexpr kFramesPerSample = 2;
    if (_s._framesSinceLastSample >= kFramesPerSample) {
        _s._posHistory.push_front(newPos);
        _s._framesSinceLastSample = 0;
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
