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
}

void FlowPlayerEntity::LoadDerived(serial::Ptree pt) {
    _selectionRadius = pt.GetFloat("selection_radius");
    _launchVel = pt.GetFloat("launch_vel");
    pt.TryGetFloat("max_horiz_speed_after_dash", &_maxHorizSpeedAfterDash);
    pt.TryGetFloat("dash_time", &_dashTime);
    serial::LoadFromChildOf(pt, "gravity", _gravity);
}

void FlowPlayerEntity::Init(GameManager& g) {
    BaseEntity::Init(g);
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

bool IsCollisionFree(GameManager& g, Vec3 const& playerPos, Vec3 const& enemyPos) {
    int numEntities = 0;
    ne::EntityManager::Iterator wallIter = g._neEntityManager->GetIterator(ne::EntityType::FlowWall, &numEntities);
    for (; !wallIter.Finished(); wallIter.Next()) {
        Mat4 const& wallTrans = wallIter.GetEntity()->_transform;
        Vec3 wallPos = wallTrans.GetPos();
        float wallMinX = wallPos._x - 0.5f*wallTrans._m00;
        float wallMaxX = wallPos._x + 0.5f*wallTrans._m00;
        float wallMinZ = wallPos._z - 0.5f*wallTrans._m22;
        float wallMaxZ = wallPos._z + 0.5f*wallTrans._m22;
        bool hit = SegmentBoxIntersection2d(
            playerPos._x, playerPos._z, enemyPos._x, enemyPos._z,
            wallMinX, wallMinZ, wallMaxX, wallMaxZ);
        if (hit) {
            return false;
        }
    }
    return true;
}
}

void FlowPlayerEntity::Draw(GameManager& g) {
    // Draw history positions
    Mat4 historyTrans;
    historyTrans.Scale(0.1f, 0.1f, 0.1f);
    for (Vec3 const& prevPos : _posHistory) {
        historyTrans.SetTranslation(prevPos);
        g._scene->DrawCube(historyTrans, _currentColor);
    }

    if (_model != nullptr) {
        g._scene->DrawMesh(_model, _transform, _currentColor);
    }
}

void FlowPlayerEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (beatTime < 3) {
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
        bool clear = IsCollisionFree(g, playerPos, enemy->_transform.GetPos());
        if (!clear) {
            enemy->_currentColor = kGreyColor;
            enemy->_textColor = kGreyColor;
            continue;
        }

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

    Draw(g);
}
