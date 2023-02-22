#include "flow_player.h"

#include "game_manager.h"
#include "typing_enemy.h"
#include "input_manager.h"
#include "renderer.h"
#include "camera.h"
#include "math_util.h"
#include "beat_clock.h"

void FlowPlayerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutFloat("selection_radius", _selectionRadius);
    pt.PutFloat("launch_vel", _launchVel);
    pt.PutFloat("max_horiz_speed_after_dash", _maxHorizSpeedAfterDash);
    pt.PutFloat("dash_time", _dashTime);
    serial::SaveInNewChildOf(pt, "gravity", _gravity);
    serial::SaveInNewChildOf(pt, "respawn_pos", _respawnPos);
}

void FlowPlayerEntity::LoadDerived(serial::Ptree pt) {
    _selectionRadius = pt.GetFloat("selection_radius");
    _launchVel = pt.GetFloat("launch_vel");
    pt.TryGetFloat("max_horiz_speed_after_dash", &_maxHorizSpeedAfterDash);
    pt.TryGetFloat("dash_time", &_dashTime);
    serial::LoadFromChildOf(pt, "gravity", _gravity);
    serial::LoadFromChildOf(pt, "respawn_pos", _respawnPos);
}

void FlowPlayerEntity::InitDerived(GameManager& g) {
    {
        int numEntities = 0;
        ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::Camera, &numEntities);
        assert(numEntities == 1);
        _cameraId = eIter.GetEntity()->_id;
    }
    _currentColor = _modelColor;
}

namespace {
float constexpr kEps = 0.0001f;

bool SegmentBoxIntersection2d(
    float sx0, float sy0, float sx1, float sy1, float minBoxX, float minBoxY, float maxBoxX, float maxBoxY) {
    float dsx = sx1 - sx0;
    float dsy = sy1 - sy0;
    // Left side of box
    {
        float col_t;
        if (std::abs(dsx) < kEps) {
            if (sx0 >= minBoxX && sx0 <= maxBoxX) {
                col_t = 0.f;
            } else {
                col_t = -1.f;
            }
        } else {
            col_t = (minBoxX - sx0) / dsx;
        }
        if (col_t >= 0.f && col_t <= 1.f) {
            float col_y = sy0 + col_t * dsy;
            if (col_y >= minBoxY && col_y <= maxBoxY) {
                return true;
            }
        }
    }

    // right side of box
    {
        float col_t;
        if (std::abs(dsx) < kEps) {
            if (sx0 >= minBoxX && sx0 <= maxBoxX) {
                col_t = 0.f;
            } else {
                col_t = -1.f;
            }
        } else {
            col_t = (maxBoxX - sx0) / dsx;
        }
        if (col_t >= 0.f && col_t <= 1.f) {
            float col_y = sy0 + col_t * dsy;
            if (col_y >= minBoxY && col_y <= maxBoxY) {
                return true;
            }
        }
    }

    // bottom side of box
    {
        float col_t;
        if (std::abs(dsy) < kEps) {
            if (sy0 >= minBoxY && sy0 <= maxBoxY) {
                col_t = 0.f;
            } else {
                col_t = -1.f;
            }
        } else {
            col_t = (minBoxY - sy0) / dsy;
        }
        if (col_t >= 0.f && col_t <= 1.f) {
            float col_x = sx0 + col_t * dsx;
            if (col_x >= minBoxX && col_x <= maxBoxX) {
                return true;
            }
        }
    }

    // top side of box
    {
        float col_t;
        if (std::abs(dsy) < kEps) {
            if (sy0 >= minBoxY && sy0 <= maxBoxY) {
                col_t = 0.f;
            } else {
                col_t = -1.f;
            }
        } else {
            col_t = (maxBoxY - sy0) / dsy;
        }
        if (col_t >= 0.f && col_t <= 1.f) {
            float col_x = sx0 + col_t * dsx;
            if (col_x >= minBoxX && col_x <= maxBoxX) {
                return true;
            }
        }
    }
    
    return false;
}

bool IsSegmentCollisionFree2D(GameManager& g, Vec3 const& playerPos, Vec3 const& enemyPos) {
    int numEntities = 0;
    ne::EntityManager::Iterator wallIter = g._neEntityManager->GetIterator(ne::EntityType::FlowWall, &numEntities);
    for (; !wallIter.Finished(); wallIter.Next()) {
        // TODO: handle rotated walls. For now we assume AABB and ignore rotation.
        Transform const& wallTrans = wallIter.GetEntity()->_transform;
        Vec3 const& wallPos = wallTrans.Pos();
        Vec3 const& scale = wallTrans.Scale();
        float wallMinX = wallPos._x - 0.5f*scale._x;
        float wallMaxX = wallPos._x + 0.5f*scale._x;
        float wallMinZ = wallPos._z - 0.5f*scale._z;
        float wallMaxZ = wallPos._z + 0.5f*scale._z;
        bool hit = SegmentBoxIntersection2d(
            playerPos._x, playerPos._z, enemyPos._x, enemyPos._z,
            wallMinX, wallMinZ, wallMaxX, wallMaxZ);
        if (hit) {
            return false;
        }
    }
    return true;
}

// Checks XZ collisions between
// NOTE: penetration points from 2 to 1.
// NOTE: penetration only has a value if this returns TRUE.
bool DoAABBsOverlap(Transform const& aabb1, Transform const& aabb2, Vec3* penetration) {
    Vec3 const& p1 = aabb1.Pos();
    Vec3 const& p2 = aabb2.Pos();    
    Vec3 halfScale1 = 0.5f * aabb1.Scale();
    Vec3 halfScale2 = 0.5f * aabb2.Scale();
    Vec3 min1 = p1 - halfScale1;
    Vec3 min2 = p2 - halfScale2;
    Vec3 max1 = p1 + halfScale1;
    Vec3 max2 = p2 + halfScale2;
    // negative is overlap
    float maxSignedDist = std::numeric_limits<float>::lowest();
    int maxSignedDistAxis = -1;
    for (int i = 0; i < 3; ++i) {
        float dist1 = min1(i) - max2(i);
        if (dist1 > 0.f) {
            return false;
        }
        float dist2 = min2(i) - max1(i);
        if (dist2 > 0.f) {
            return false;
        }
        float maxDist = std::max(dist1, dist2);
        if (maxDist > maxSignedDist) {
            maxSignedDistAxis = i;
            maxSignedDist = maxDist;
        }
    }
    if (penetration) {
        assert(maxSignedDistAxis >= 0);
        assert(maxSignedDist <= 0);
        float sign = p1(maxSignedDistAxis) >= p2(maxSignedDistAxis) ? -1.f : 1.f;
        (*penetration)(maxSignedDistAxis) = sign * maxSignedDist;
    }
    return true;
}

// penetration is offset to move player to move out of (one) collision.
// NOTE: penetration only populated if returns true
bool DoesPlayerOverlapAnyWallNoRotate(GameManager& g, Transform const& playerTrans, Vec3* penetrationOut) {
    Vec3 penetration;
    ne::EntityManager::Iterator wallIter = g._neEntityManager->GetIterator(ne::EntityType::FlowWall);
    for (; !wallIter.Finished(); wallIter.Next()) {
        Transform const& wallTrans = wallIter.GetEntity()->_transform;
        bool hasOverlap = DoAABBsOverlap(playerTrans, wallTrans, &penetration);
        if (hasOverlap) {
            *penetrationOut = penetration;
            return true;
        }
    }
    return false;
}
}

void FlowPlayerEntity::Draw(GameManager& g) {
    // Draw history positions
    Mat4 historyTrans;
    historyTrans.Scale(0.1f, 0.1f, 0.1f);
    for (Vec3 const& prevPos : _posHistory) {
        historyTrans.SetTranslation(prevPos);
        renderer::ColorModelInstance& model = g._scene->DrawCube(historyTrans, _currentColor);
        model._topLayer = true;
    }

    if (_model != nullptr) {
        renderer::ColorModelInstance& model = g._scene->DrawMesh(_model, _transform.Mat4Scale(), _currentColor);
        model._topLayer = true;
    }
}

void FlowPlayerEntity::Respawn(GameManager& g) {
    _transform.SetTranslation(_respawnPos);
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    _countOffEndTime = g._beatClock->GetNextDownBeatTime(beatTime) + 3;
    _posHistory.clear();
    _framesSinceLastSample = 0;
    _vel.Set(0.f,0.f,0.f);
    _dashTimer = -1.f;
    
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
        Draw(g);
        return;
    } else {
        _currentColor = _modelColor;
    }

    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    Vec3 const& playerPos = _transform.GetPos();
ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy);
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        Vec4 constexpr kGreyColor(0.6f, 0.6f, 0.6f, 0.7f);
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
        Vec3 dp = playerPos - enemy->_transform.GetPos();
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

        InputManager::Key nextKey = enemy->GetNextKey();
        if (!g._inputManager->IsKeyPressedThisFrame(nextKey)) {
            continue;
        }

        if (nearest == nullptr || d2 < nearestDist2) {
            nearest = enemy;
            nearestDist2 = d2;
        }
    }

    if (nearest != nullptr) {
        nearest->OnHit(g);
        Vec3 toEnemyDir = nearest->_transform.GetPos() - playerPos;
        toEnemyDir.Normalize();
        float sign = (_flowPolarity != nearest->_flowPolarity) ? 1.f : -1.f;
        _vel = toEnemyDir * (sign * _launchVel);
        _dashTimer = 0.f;
        _dashTargetId = nearest->_id;

        if (nearest->_flowSectionId != _currentSectionId) {
            // go through and destroy all enemies with the current section id
            ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy);
            for (; !enemyIter.Finished(); enemyIter.Next()) {
                TypingEnemyEntity* e = static_cast<TypingEnemyEntity*>(enemyIter.GetEntity());
                if (e->_flowSectionId >= 0 && e->_flowSectionId == _currentSectionId) {
                    g._neEntityManager->TagForDestroy(enemyIter.GetEntity()->_id);
                }
            }
            _currentSectionId = nearest->_flowSectionId;
        }
    }

    if (_dashTimer >= _dashTime) {
        // Finished dashing. reset vel.
        _vel._x = math_util::Clamp(_vel._x, -_maxHorizSpeedAfterDash, _maxHorizSpeedAfterDash);
        _vel._z = 0.f;
        _dashTimer = -1.f;
    } else if (_dashTimer >= 0.f) {
        // Check if we've "passed" the dash target. If so, shorten the dash time..
        if (ne::Entity* dashTarget = g._neEntityManager->GetEntity(_dashTargetId)) {
            Vec3 playerPos = _transform.Pos();
            Vec3 targetPos = dashTarget->_transform.Pos();
            Vec3 prevOffset = targetPos - playerPos;
            playerPos += _vel * dt;
            Vec3 nextOffset = targetPos - playerPos;
            float dotp = Vec3::Dot(prevOffset, nextOffset);
            if (dotp < 0.f) {
                _dashTimer = std::max(_dashTimer, _dashTime - 2*dt);
            }
        }

        // Still dashing. maintain velocity.
        _dashTimer += dt;
    } else {
        // Not dashing. Apply gravity.
        _vel += _gravity * dt;
    }

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
    if (DoesPlayerOverlapAnyWallNoRotate(g, _transform, &penetration)) {
        // Respawn(g);
        p += penetration;
        Vec3 collNormal = penetration.GetNormalized();
        Vec3 newVel = -_vel;
        // reflect across coll normal
        Vec3 tangentPart = Vec3::Dot(newVel, collNormal) * collNormal;
        Vec3 normalPart = newVel - tangentPart;
        newVel -= 2 * normalPart;

        _transform.SetTranslation(p);
        _vel = newVel;
    }

    Draw(g);
}
