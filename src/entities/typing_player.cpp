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
        case EnemySelectionType::MinXZ: selectionTypeStr = "MinXZ"; break;
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
        } else if (selectionTypeStr == "MinXZ")
            _selectionType = EnemySelectionType::MinXZ;
        else {
            printf("TypingPlayerEntity::LoadDerived: ERROR unrecognized selection type \"%s\"", selectionTypeStr.c_str());
        }
    }
    if (_selectionType == EnemySelectionType::NearPos) {
        _selectionRadius = pt.GetFloat("selection_radius");
    }
}

void TypingPlayerEntity::Init(GameManager& g) {   
    _sectionNumBeats[-1] = 4;
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

    int constexpr kNumVisibleSections = 2;
    
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    Vec3 topSectionOrigin(-8.f, 0.f, -4.f);
    double constexpr kScrollTime = 1.0;
    float constexpr kDistBetweenSections = 5.f;
    // TODO OMG CLEAN THIS SHIT UP
    if (beatTime >= _nextSectionStartTime) {
        // This is currently handled by the dirty hack below
        // _transform.SetTranslation(Vec3(-10.f, 0.f, 0.f));
        
        std::vector<ne::EntityId> toRemove;
        {
            std::set<ne::EntityId>& sectionEnemies = _sectionEnemies[_currentSectionId];
            for (ne::EntityId const eId : sectionEnemies) {
                toRemove.push_back(eId);
            }
        }

        ++_currentSectionId;
        _nextSectionStartTime += _sectionNumBeats[_currentSectionId];
        
        Vec3 sectionOrigin = topSectionOrigin;
        for (int sectionIx = 0; sectionIx < kNumVisibleSections; ++sectionIx) {
            int sectionId = _currentSectionId + sectionIx;
            std::set<ne::EntityId>& sectionEnemies = _sectionEnemies[sectionId];
            if (sectionIx >= 0) {
                for (ne::EntityId const eId : sectionEnemies) {
                    TypingEnemyEntity* enemy = static_cast<TypingEnemyEntity*>(g._neEntityManager->GetEntity(eId));
                    if (enemy == nullptr) {
                        toRemove.push_back(eId);
                        continue;
                    }
                    // instant activate
                    enemy->_activeBeatTime = -1.0;
                    Vec3 p = sectionOrigin + enemy->_sectionLocalPos;
                    enemy->_transform.SetTranslation(p);
                    if (sectionIx > 0) {
                        enemy->_currentColor.Set(0.2f, 0.2f, 0.2f, 1.f);
                    } else {
                        enemy->_currentColor = enemy->_modelColor;
                    }
                }
            }            
            sectionOrigin._z += kDistBetweenSections;
        }

        for (ne::EntityId const eId : toRemove) {
            g._neEntityManager->TagForDestroy(eId);
        }
        toRemove.clear();
        _adjustedForQuant = false;
        
    } else if (beatTime + kScrollTime >= _nextSectionStartTime) {
        double param = 1 - (_nextSectionStartTime - beatTime) / kScrollTime;
        param = math_util::Clamp(param, 0.0, 1.0);
        topSectionOrigin._z -= param * kDistBetweenSections;
                
        Vec3 sectionOrigin = topSectionOrigin;
        for (int sectionIx = 0; sectionIx < kNumVisibleSections + 1; ++sectionIx) {
            int sectionId = _currentSectionId + sectionIx;
            if (sectionId >= 0) {
                std::set<ne::EntityId>& sectionEnemies = _sectionEnemies[sectionId];
                for (ne::EntityId const eId : sectionEnemies) {
                    TypingEnemyEntity* enemy = static_cast<TypingEnemyEntity*>(g._neEntityManager->GetEntity(eId));
                    if (enemy == nullptr) {
                        continue;
                    }
                    // instant activate
                    enemy->_activeBeatTime = -1.0;
                    Vec3 p = sectionOrigin + enemy->_sectionLocalPos;
                    enemy->_transform.SetTranslation(p);
                    if (sectionIx > 0) {
                        enemy->_currentColor.Set(0.2f, 0.2f, 0.2f, 1.f);
                    } else {
                        enemy->_currentColor = enemy->_modelColor;
                    }
                }
            }
            sectionOrigin._z += kDistBetweenSections;
        }
    }

    // Draw section counters and dividers
    {
        Vec3 sectionOrigin = topSectionOrigin;
        for (int sectionIx = 0; sectionIx < kNumVisibleSections; ++sectionIx) {
            // Draw counters
            int sectionId = _currentSectionId + sectionIx;
            auto numBeatsIter = _sectionNumBeats.find(sectionId);
            if (numBeatsIter != _sectionNumBeats.end()) {
                int beatsLeft = (int)(_nextSectionStartTime - beatTime);
                int numBeats = numBeatsIter->second;
                int currentBeatIx = numBeats - beatsLeft - 1;
                float counterSize = (kDistBetweenSections - 1) / numBeats;
                for (int beatIx = 0; beatIx < numBeats; ++beatIx) {
                    Mat4 trans;
                    trans.SetTranslation(sectionOrigin);
                    trans._m03 -= 1.f;
                    trans._m23 += beatIx * counterSize;
                    trans.Scale(0.25f, 0.25f, counterSize * 0.9f);
                    Vec4 color;
                    if (sectionIx == 0 && beatIx <= currentBeatIx) {
                        color.Set(1.f, 1.f, 0.f, 1.f);
                    } else {
                        color.Set(0.1f, 0.1f, 0.1f, 1.f);
                    }
                    g._scene->DrawCube(trans, color);
                }
            }
            
            Mat4 trans;
            trans.SetTranslation(Vec3(0.f, 0.f, sectionOrigin._z + kDistBetweenSections - 1.0));
            trans.Scale(20.f, 0.25f, 0.25f);
            Vec4 color(0.1f, 0.1f, 0.1f, 1.f);
            g._scene->DrawCube(trans, color);
            sectionOrigin._z += kDistBetweenSections;
        }
    }
    
    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy);
    Vec3 const& playerPos = _transform.GetPos();
    // ***DIRTY HACK*** account for note quantization so we can hit downbeats reliably
    // TODO: try to do this instead by doing all this checking in the StepSequencer?
    int effectiveSectionId = _currentSectionId;
    {
        if (beatTime + 0.25 > _nextSectionStartTime) {
            ++effectiveSectionId;
            if (!_adjustedForQuant) {
                _transform.SetTranslation(Vec3(-10.f, 0.f, 0.f));
                _adjustedForQuant = true;
            }
        }
    }
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
        if (!enemy->IsActive(g)) {
            continue;
        }
        if (enemy->_sectionId >= 0 && effectiveSectionId != enemy->_sectionId) {
            continue;
        }
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
                case EnemySelectionType::MinXZ: {
                    int dId = enemy->_id._id - _currentEnemyId._id;
                    if (dId < 0.f) { // if enemy is before player
                        dist2 = 100.f * (-dId);
                    } else {
                        dist2 = (float) enemy->_id._id;
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

void TypingPlayerEntity::RegisterSectionEnemy(int sectionId, ne::EntityId enemyId) {
    _sectionEnemies[sectionId].insert(enemyId);
}

void TypingPlayerEntity::SetSectionLength(int sectionId, int numBeats) {
    _sectionNumBeats[sectionId] = numBeats;
}
