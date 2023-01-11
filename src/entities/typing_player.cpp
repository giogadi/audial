#include "typing_player.h"

#include "input_manager.h"
#include "game_manager.h"
#include "entities/typing_enemy.h"
#include "constants.h"
#include "renderer.h"
#include "math_util.h"

void TypingPlayerEntity::SaveDerived(serial::Ptree pt) const {
    std::string selectionTypeStr;
    switch (_selectionType) {
        case EnemySelectionType::MinX: selectionTypeStr = "MinX"; break;
        case EnemySelectionType::NearPos: selectionTypeStr = "NearPos"; break;
    }
    pt.PutString("enemy_selection_type", selectionTypeStr.c_str());
    if (_selectionType == EnemySelectionType::NearPos) {
        pt.PutFloat("selection_radius", _selectionRadius);
    }
}

void TypingPlayerEntity::LoadDerived(serial::Ptree pt) {
    _selectionType = EnemySelectionType::MinX;
    std::string selectionTypeStr;
    if (pt.TryGetString("enemy_selection_type", &selectionTypeStr)) {
        if (selectionTypeStr == "MinX") {
            _selectionType = EnemySelectionType::MinX;
        } else if (selectionTypeStr == "NearPos") {
            _selectionType = EnemySelectionType::NearPos;
        } else {
            printf("TypingPlayerEntity::LoadDerived: ERROR unrecognized selection type \"%s\"", selectionTypeStr.c_str());
        }
    }
    if (_selectionType == EnemySelectionType::NearPos) {
        _selectionRadius = pt.GetFloat("selection_radius");
    }
}

void TypingPlayerEntity::Update(GameManager& g, float dt) {
    // Draw rhythm bar
    // {
    //     float xPos = -8.f;
    //     Vec4 color(0.f, 0.f, 0.f, 1.f);
    //     Mat4 transform;
    //     transform.SetTranslation(Vec3(xPos, 0.f, 0.f));
    //     transform.Scale(0.25f, 0.25f, 25.f);
    //     g._scene->DrawCube(transform, color);
    // }
    
    if (g._editMode) {
        return;
    }    
    
    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy);
    Vec3 const& playerPos = _transform.GetPos();
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
        if (!enemy->IsActive(g)) {
            continue;
        }
        // if (enemy->_numHits >= enemy->_text.length()) {
        //     continue;
        // }
        InputManager::Key nextKey = enemy->GetNextKey();
        if (g._inputManager->IsKeyPressedThisFrame(nextKey)) {
            std::optional<float> dist2;
            switch (_selectionType) {
                case EnemySelectionType::MinX:
                    dist2 = enemy->_transform.GetPos()._x;
                    break;
                case EnemySelectionType::NearPos: {
                    Vec3 dp = playerPos - enemy->_transform.GetPos();
                    float d2 = dp.Length2();
                    if (d2 <= _selectionRadius * _selectionRadius) {
                        dist2 = d2;
                    }
                    break;
                }
                    
            }
            if (!dist2.has_value()) {
                continue;
            }
            if (nearest == nullptr) {
                nearest = enemy;
                nearestDist2 = *dist2;
            } else if (*dist2 < nearestDist2) {
                nearest = enemy;
                nearestDist2 = *dist2;
            }
        }
    }

    if (nearest != nullptr) {
        if (TypingEnemyEntity* previousEnemy = static_cast<TypingEnemyEntity*>(g._neEntityManager->GetEntity(_currentEnemyId))) {
            previousEnemy->_currentColor = previousEnemy->_modelColor;
        }
        
        nearest->OnHit(g);
        _transform.SetTranslation(nearest->_transform.GetPos());
        _currentEnemyId = nearest->_id;
    }

    TypingEnemyEntity* currentEnemy = static_cast<TypingEnemyEntity*>(g._neEntityManager->GetEntity(_currentEnemyId));
    if (currentEnemy) {
        double beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double unusedBeatNumber;
        double beatTimeFrac = modf(beatTime, &unusedBeatNumber);
        //Vec4 enemyColor = currentEnemy->_modelColor;
        Vec4 enemyColor(0.2f, 0.2f, 0.2f, 1.f);
        constexpr Vec4 selectionColor(0.f, 1.f, 0.f, 1.f);
        // as beatTimeFrac goes from 0 to 0.5, go from selectionColor to
        // enemyColor, and then back to selectionColor from 0.5 to 1.
        float param = math_util::SmoothUpAndDown((float)beatTimeFrac);
        currentEnemy->_currentColor = selectionColor + param * (enemyColor - selectionColor);
    }
}
