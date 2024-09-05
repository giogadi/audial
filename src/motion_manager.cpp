#include "motion_manager.h"

#include <cstdio>

#include "new_entity.h"

namespace {
	// returns true if swapped something else in
	bool SwapRemove(Motion* motions, int* count, int removeIx) {
		if (removeIx + 1 < *count) {
			motions[removeIx] = motions[*count - 1];
			*count -= 1;
			return true;
		} else {
			*count -= 1;
			return false;
		}
	}
}

Motion* MotionManager::AddMotion(ne::EntityId entityId) {
	if (_motionCount + 1 >= kMaxMotions) {
		printf("MotionManager: EXCEEDED MAX MOTIONS\n");
		return nullptr;
	}
	Motion* motion = &_motions[_motionCount++];
	motion->entityId = entityId;
	return motion;
}

void MotionManager::Update(float dt, GameManager& g) {
	for (int ii = 0; ii < _motionCount; ++ii) {
		Motion* motion = &_motions[ii];
		bool needRemove = false;
		motion->timeLeft -= dt;
		if (motion->timeLeft <= 0.f) {
			needRemove = true;
		}
		ne::BaseEntity* entity = nullptr;
		if (!needRemove) {
			entity = g._neEntityManager->GetEntity(motion->entityId);
			if (entity == nullptr) {
				needRemove = true;
			}
		}
		if (needRemove) {
			if (SwapRemove(_motions, &_motionCount, ii)) {
				--ii;
			}
			continue;
		}
		Vec3 dp = motion->v * dt;
		entity->_transform.Translate(dp);
	}
}