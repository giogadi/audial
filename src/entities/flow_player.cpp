#include "flow_player.h"

#include "game_manager.h"
#include "typing_enemy.h"
#include "input_manager.h"
#include "renderer.h"

void FlowPlayerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutFloat("selection_radius", _selectionRadius);
    pt.PutFloat("launch_vel", _launchVel);
    pt.PutFloat("decel", _decel);  
}

void FlowPlayerEntity::LoadDerived(serial::Ptree pt) {
    _selectionRadius = pt.GetFloat("selection_radius");
    _launchVel = pt.GetFloat("launch_vel");
    _decel = pt.GetFloat("decel");
}

void FlowPlayerEntity::Init(GameManager& g) {
    BaseEntity::Init(g);
    _transform.Scale(0.5f, 0.5f, 0.5f);
}

void FlowPlayerEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    Vec3 const& playerPos = _transform.GetPos();
ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy);
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
        InputManager::Key nextKey = enemy->GetNextKey();
        if (!g._inputManager->IsKeyPressedThisFrame(nextKey)) {
            continue;
        }
        Vec3 dp = playerPos - enemy->_transform.GetPos();
        float d2 = dp.Length2();
        if (nearest == nullptr || _selectionRadius < 0.f || d2 <= _selectionRadius * _selectionRadius) {
            nearest = enemy;
            nearestDist2 = d2;
        }
    }

    if (nearest != nullptr) {
        nearest->OnHit(g);
        Vec3 toEnemyDir = nearest->_transform.GetPos() - playerPos;
        toEnemyDir.Normalize();
        _vel = toEnemyDir * _launchVel;
    }

    float constexpr kEps = 0.0001f;
    if (std::abs(_vel._x) < kEps && std::abs(_vel._y) < kEps && std::abs(_vel._z) < kEps) {
        _vel.Set(0.f, 0.f, 0.f);
    } else {
        float speed = _vel.Length();
        float newSpeed = speed - dt * _decel;
        if ((newSpeed > 0) != (speed > 0)) {
            newSpeed = 0.f;
        }
        _vel = _vel * (newSpeed / speed);
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

    // Draw history positions
    Mat4 historyTrans;
    historyTrans.Scale(0.25f, 0.25f, 0.25f);
    for (Vec3 const& prevPos : _posHistory) {
        historyTrans.SetTranslation(prevPos);
        g._scene->DrawCube(historyTrans, _modelColor);
    }

    BaseEntity::Update(g, dt);    
}
