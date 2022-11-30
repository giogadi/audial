#include "typing_player.h"

#include <cctype>

#include "input_manager.h"
#include "game_manager.h"
#include "entities/typing_enemy.h"

void TypingPlayerEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }
    
    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy);
    Vec3 const& playerPos = _transform.GetPos();
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
        if (enemy->_numHits >= enemy->_text.length()) {
            continue;
        }
        char nextChar = std::tolower(enemy->_text[enemy->_numHits]);
        int charIx = nextChar - 'a';
        if (charIx < 0 || charIx > static_cast<int>(InputManager::Key::Z)) {
            printf("WARNING: char \'%c\' not in InputManager!\n", nextChar);
            continue;
        }
        InputManager::Key nextKey = static_cast<InputManager::Key>(charIx);
        if (g._inputManager->IsKeyPressedThisFrame(nextKey)) {
            if (nearest == nullptr) {
                nearest = enemy;
            } else {
                Vec3 dp = playerPos - enemy->_transform.GetPos();
                float dist2 = dp.Length2();
                if (dist2 < nearestDist2) {
                    nearest = enemy;
                    nearestDist2 = dist2;
                }
            }
        }
    }

    if (nearest != nullptr) {
        nearest->OnHit(g);
    }
}
