#include "entities/player.h"

#include <array>

#include "game_manager.h"
#include "input_manager.h"
#include "entities/enemy.h"
#include "renderer.h"

namespace {
void DrawLanes(GameManager& g) {
    int constexpr kNumLanes = 8;
    float constexpr kLaneWidth = 2.2f;
    float constexpr kLaneBoundaryWidth = 0.25f;
    float startX = -0.5f * kNumLanes * kLaneWidth;
    Vec4 color(0.f, 0.f, 0.f, 1.f);
    for (int i = 0; i < kNumLanes+1; ++i) {
        Mat4 transform;
        transform.SetTranslation(Vec3(startX, 0.f, -2.f));
        transform.Scale(kLaneBoundaryWidth, kLaneBoundaryWidth, 20.f);
        g._scene->DrawCube(transform, color);
        startX += kLaneWidth;
    }
}
}

void PlayerEntity::Update(GameManager& g, float dt) {
    DrawLanes(g);

    // assume only a few keys can be pressed at once.
    int constexpr kNumSimulKeys = 4;
    std::array<InputManager::Key, kNumSimulKeys> keysShot;
    int numKeysShot = 0;
    ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::Enemy);
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        EnemyEntity* enemy = (EnemyEntity*) enemyIter.GetEntity();
        if (!enemy->IsActive(g)) {
            continue;
        }
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