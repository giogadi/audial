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
    pt.PutFloat("selection_radius", _selectionRadius);
    pt.PutFloat("launch_vel", _launchVel);
    pt.PutFloat("max_horiz_speed_after_dash", _maxHorizSpeedAfterDash);
    pt.PutFloat("dash_time", _dashTime);
    serial::SaveInNewChildOf(pt, "gravity", _gravity);
    serial::SaveInNewChildOf(pt, "respawn_pos", _respawnPos);
    pt.PutFloat("max_speed", _maxSpeed);
}

void FlowPlayerEntity::LoadDerived(serial::Ptree pt) {
    _selectionRadius = pt.GetFloat("selection_radius");
    _launchVel = pt.GetFloat("launch_vel");
    pt.TryGetFloat("max_horiz_speed_after_dash", &_maxHorizSpeedAfterDash);
    pt.TryGetFloat("dash_time", &_dashTime);
    serial::LoadFromChildOf(pt, "gravity", _gravity);
    serial::LoadFromChildOf(pt, "respawn_pos", _respawnPos);
    _maxSpeed = 20.f;
    pt.TryGetFloat("max_speed", &_maxSpeed);
}

void FlowPlayerEntity::InitDerived(GameManager& g) {
    {
        int numEntities = 0;
        ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::Camera, &numEntities);
        assert(numEntities == 1);
        _cameraId = eIter.GetEntity()->_id;
    }
    _currentColor = _modelColor;
    _transform.SetPos(_respawnPos);
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

void FlowPlayerEntity::DrawPlayer(GameManager& g) {
    // Draw history positions
    Mat4 historyTrans;
    historyTrans.Scale(0.1f, 0.1f, 0.1f);
    for (Vec3 const& prevPos : _posHistory) {
        historyTrans.SetTranslation(prevPos);
        renderer::ModelInstance& model = g._scene->DrawCube(historyTrans, _currentColor);
         model._topLayer = true;
    }

    if (_model != nullptr) {
        renderer::ModelInstance& model = g._scene->DrawMesh(_model, _transform.Mat4Scale(), _currentColor);
         model._topLayer = true;
    }

    if (_dashTimer >= 0.f) {
        if (ne::Entity* dashTarget = g._neEntityManager->GetEntity(_dashTargetId)) {
            Vec3 targetPos = dashTarget->_transform.Pos();
            g._scene->DrawLine(_transform.Pos(), targetPos, dashTarget->_modelColor);
        }
    }
}

void FlowPlayerEntity::Respawn(GameManager& g) {
    _transform.SetTranslation(_respawnPos);
    // double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    // _countOffEndTime = g._beatClock->GetNextDownBeatTime(beatTime) + 3;
    _posHistory.clear();
    _framesSinceLastSample = 0;
    _vel.Set(0.f,0.f,0.f);
    _dashTimer = -1.f;
    _respawnBeforeFirstDash = true;

    // Reset everything from the current section
    for (ne::EntityManager::AllIterator iter = g._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity* e = iter.GetEntity();
        if (e->_flowSectionId >= 0 && e->_flowSectionId == _currentSectionId) {
            e->Init(g);
        }
    }
    // Reactivate all inactive entities that were initially active    
    for (ne::EntityManager::AllIterator iter = g._neEntityManager->GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity* e = iter.GetEntity();
        if (e->_initActive && e->_flowSectionId >= 0 && e->_flowSectionId == _currentSectionId) {
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
    if (ne::Entity* e = g._neEntityManager->GetEntity(_toActivateOnRespawn, /*includeActive=*/false, /*includeInactive=*/true)) {
        g._neEntityManager->TagForActivate(e->_id, /*initOnActivate=*/true);
    }
    else if (ne::Entity* e = g._neEntityManager->GetEntity(_toActivateOnRespawn)) {
        e->Init(g);
    }
}

void FlowPlayerEntity::SetNewSection(GameManager& g, int newSectionId) {
    _currentSectionId = newSectionId;
}

void FlowPlayerEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (_countOffEndTime >= 0.0 && beatTime < _countOffEndTime) {
        double unusedBeatNum;
        float beatTimeFrac = (float) (modf(beatTime, &unusedBeatNum));
        // 0 is default color, 1 is temp flash color
        float factor = math_util::SmoothStep(beatTimeFrac);
        Vec4 constexpr kFlashColor(0.f, 0.f, 0.f, 1.f);
        _currentColor = _modelColor + factor * (kFlashColor - _modelColor);
        DrawPlayer(g);
        return;
    } else {
        _currentColor = _modelColor;
    }

    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    Vec3 const& playerPos = _transform.GetPos();    
    Mat4 viewProjTransform = g._scene->GetViewProjTransform();
    bool const usingController = g._inputManager->IsUsingController();
    for (ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !enemyIter.Finished(); enemyIter.Next()) {
        Vec4 constexpr kGreyColor(0.6f, 0.6f, 0.6f, 0.7f);
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
        Vec3 dp = playerPos - enemy->_transform.GetPos();
        dp._y = 0.f;
        float d2 = dp.Length2();
        if (_selectionRadius >= 0.f && d2 > _selectionRadius * _selectionRadius) {
            enemy->_currentColor = kGreyColor;
            enemy->_textColor = kGreyColor;
            continue;
        }
        // bool clear = IsCollisionFree(g, playerPos, enemy->_transform.GetPos());
        // if (!clear) {
        //     enemy->_currentColor = kGreyColor;
        //     enemy->_textColor = kGreyColor;
        //     continue;
        // }

        // UGLY
        enemy->_currentColor = enemy->_modelColor;
        enemy->_textColor.Set(1.f, 1.f, 1.f, 1.f);

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
        ne::EntityId nearestId = nearest->_id;
        if (nearest->CanHit()) {
            _respawnBeforeFirstDash = false;
            nearest->OnHit(g);
            Vec3 toEnemyDir = nearest->_transform.GetPos() - playerPos;
            toEnemyDir._y = 0.f;
            toEnemyDir.Normalize();
            float sign = (_flowPolarity != nearest->_flowPolarity) ? 1.f : -1.f;
            _vel = toEnemyDir * (sign * _launchVel);
            _dashTimer = 0.f;
            _dashTargetId = nearest->_id;
            _applyGravityDuringDash = false;
            
            for (auto enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !enemyIter.Finished(); enemyIter.Next()) {
                TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
                if (enemy->_id != nearestId) {
                    enemy->OnHitOther(g);
                }
            }

            // for (auto vfxIter = g._neEntityManager->GetIterator(ne::EntityType::Vfx); !vfxIter.Finished(); vfxIter.Next()) {
            //     VfxEntity* vfx = (VfxEntity*) vfxIter.GetEntity();
            //     vfx->OnHit(g);
            // }
        }
    }

    // Check if we're done dashing
    {
        bool doneDashing = false;
        bool passedDashTarget = false;
        if (_dashTimer >= _dashTime) {
            doneDashing = true;
        } else if (_dashTimer >= 0.f) {
            if (ne::Entity* dashTarget = g._neEntityManager->GetEntity(_dashTargetId)) {
                Vec3 playerPos = _transform.Pos();
                Vec3 targetPos = dashTarget->_transform.Pos();
                targetPos._y = playerPos._y;
                Vec3 prevOffset = targetPos - playerPos;
                playerPos += _vel * dt;
                Vec3 nextOffset = targetPos - playerPos;
                float dotp = Vec3::Dot(prevOffset, nextOffset);
                if (dotp < 0.f) {
                    doneDashing = true;
                    passedDashTarget = true;
                }
            }
        }

        if (doneDashing) {
            if (passedDashTarget) {
                constexpr float kPassTargetSpeed = 2.f;
                float speed = _vel.Normalize();
                speed = std::min(speed, kPassTargetSpeed);
                _vel *= speed;
            } else {
                _vel._x = math_util::Clamp(_vel._x, -_maxHorizSpeedAfterDash, _maxHorizSpeedAfterDash);
                _vel._z = 0.f;
            }
            _dashTimer = -1.f;
            _dashTargetId = ne::EntityId();
        }
    }
    
    // Update state based on dash status
    if (_dashTimer >= 0.f) {
        if (_applyGravityDuringDash) {
            _vel += _gravity * dt;
        }

        // Still dashing. maintain velocity.
        _dashTimer += dt;
    } else {
        // Not dashing. Apply gravity if we've dashed since respawn.
        if (!_respawnBeforeFirstDash) {
            _vel += _gravity * dt;
            _vel._y = std::max(-_maxSpeed, _vel._y);
        }
    }

    // limit to _maxSpeed
    // {
    //     float speed = std::min(_vel.Length(), _maxSpeed);
    //     _vel.Normalize();
    //     _vel *= speed;
    // }

    Vec3 p = _transform.GetPos();

    // Update prev positions
    int constexpr kMaxHistory = 5;
    if (_posHistory.size() == kMaxHistory) {
        _posHistory.pop_back();
    }

    ++_framesSinceLastSample;
    int constexpr kFramesPerSample = 2;
    if (_framesSinceLastSample >= kFramesPerSample) {
        _posHistory.push_front(p);
        _framesSinceLastSample = 0;
    }

    p += _vel * dt;
    _transform.SetTranslation(p);

    // Check collisions between p and walls    
    Vec3 penetration;
    FlowWallEntity* pHitWall = FindOverlapWithWall(g, _transform, &penetration);
    penetration._y = 0.f;
    if (pHitWall != nullptr) {
        // Respawn(g);
        pHitWall->OnHit(g);
        p += penetration * 1.1f;  // Add some padding to ensure we're outta collision
        Vec3 collNormal = penetration.GetNormalized();
        Vec3 newVel = -_vel;
        // reflect across coll normal
        Vec3 tangentPart = Vec3::Dot(newVel, collNormal) * collNormal;
        Vec3 normalPart = newVel - tangentPart;
        newVel -= 2 * normalPart;        

        // after bounce, ensure we have some velocity in the collision normal
        float constexpr kMinBounceSpeed = 20.f;
        float speedAlongNormal = Vec3::Dot(newVel, collNormal);
        if (speedAlongNormal < kMinBounceSpeed) {
            Vec3 correction = (kMinBounceSpeed - speedAlongNormal) * collNormal;
            newVel += correction;
        }        

        _transform.SetTranslation(p);
        _vel = newVel;

        // Pretend it's a new dash with gravity applied
        _dashTimer = 0.5f * _dashTime;
        _applyGravityDuringDash = false;
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
    if (_killMaxZ.has_value()) {
        Vec3 p = _transform.Pos();
        if (p._z > _killMaxZ.value()) {
            Respawn(g);
            respawned = true;
        }
    }
    if (!respawned && (_killIfBelowCameraView || _killIfLeftOfCameraView)) {
        Vec3 p = _transform.Pos();
        float playerSize = _transform.Scale()._x;
        float screenX, screenY;
        geometry::ProjectWorldPointToScreenSpace(p, viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);
        if (_killIfBelowCameraView && screenY > g._windowHeight + 4 * playerSize) {
            Respawn(g);
            respawned = true;
        } else if (_killIfLeftOfCameraView && screenX < -4 * playerSize) {
            Respawn(g);
            respawned = true;
        }
    }

    DrawPlayer(g);
}
