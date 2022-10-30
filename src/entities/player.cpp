#include "entities/player.h"

#include <array>

#include "game_manager.h"
#include "input_manager.h"
#include "entities/enemy.h"

void PlayerEntity::Update(GameManager& g, float dt) {
    // assume only a few keys can be pressed at once.
    int constexpr kNumSimulKeys = 4;
    std::array<InputManager::Key, kNumSimulKeys> keysShot;
    int numKeysShot = 0;
    ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::Enemy);
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        EnemyEntity* enemy = (EnemyEntity*) enemyIter.GetEntity();
        InputManager::Key enemyKey = enemy->_shootButton;
        if (!g._inputManager->IsKeyPressedThisFrame(enemyKey)) {
            continue;
        }
        auto keysEnd = keysShot.begin() + numKeysShot;
        auto findRes = std::find(keysShot.begin(), keysEnd, enemyKey);
        if (findRes != keysEnd) {
            // Already shot an enemy with this key, skip.
            continue;
        }
        keysShot[numKeysShot] = enemyKey;
        ++numKeysShot;
        enemy->OnShot(g);
        if (numKeysShot >= kNumSimulKeys) {
            break;
        }
    }
}