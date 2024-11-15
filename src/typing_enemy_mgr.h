#pragma once

#include <unordered_map>
#include <unordered_set>

#include "new_entity_id_hash.h"
#include "game_manager.h"
#include "entities/typing_enemy.h"

struct TypingEnemyGroup {
	std::unordered_set<ne::EntityId> enemies;
};

struct TypingEnemyMgr {
	std::unordered_map<int, TypingEnemyGroup> groups;
};

void TypingEnemyMgr_AddEnemy(TypingEnemyMgr &mgr, GameManager &g, TypingEnemyEntity &e);
void TypingEnemyMgr_Update(TypingEnemyMgr &mgr, GameManager &g);