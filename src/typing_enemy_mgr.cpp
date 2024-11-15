#include "typing_enemy_mgr.h"

void TypingEnemyMgr_AddEnemy(TypingEnemyMgr &mgr, GameManager &g, TypingEnemyEntity &e) {
	int enemyGroupId = e._p._groupId;
	if (enemyGroupId < 0) {
		return;
	}
	auto result = mgr.groups.emplace(enemyGroupId, TypingEnemyGroup());
	result.first->second.enemies.insert(e._id);
}
void TypingEnemyMgr_Update(TypingEnemyMgr &mgr, GameManager &g) {
	for (auto iter = mgr.groups.begin(); iter != mgr.groups.end(); /*no inc*/) {
		TypingEnemyGroup &group = iter->second;
		bool sawAnEnemy = false;
		bool allOnCooldown = true;
		for (ne::EntityId enemyId : group.enemies) {
			bool active;
			TypingEnemyEntity *e = g._neEntityManager->GetEntityAs<TypingEnemyEntity>(enemyId, &active);
			if (e) {
				sawAnEnemy = true;
				if (active) {
					if (e->_s._flowCooldownStartBeatTime < 0.0) {
						allOnCooldown = false;
						break;
					}
				}
			}
		}
		if (!sawAnEnemy) {
			iter = mgr.groups.erase(iter);
			continue;
		}
		if (allOnCooldown) {
			for (ne::EntityId enemyId : group.enemies) {
				if (TypingEnemyEntity *e = g._neEntityManager->GetEntityAs<TypingEnemyEntity>(enemyId)) {
					e->ResetCooldown();
				}
			}
		}
		++iter;
	}
}