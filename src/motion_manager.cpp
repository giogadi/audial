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
	*motion = Motion();
	motion->entityId = entityId;
	return motion;
}

void MotionManager::Update(float dt, GameManager& g) {
	for (int ii = 0; ii < _motionCount; ++ii) {
		Motion* motion = &_motions[ii];
		bool needRemove = false;
		motion->timeLeft -= dt;
		if (motion->timeLeft <= 0.f) {
			if (g._editMode) {
				if (motion->timeLeft < -1.f) {
					ne::BaseEntity* e = g._neEntityManager->GetEntity(motion->entityId);
					if (e) {
						e->_transform = e->_initTransform;
					}
					needRemove = true;
				} else {
					continue;
				}
			} else {
				needRemove = true;
			}
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
        Vec3 dv = motion->a * dt;
        motion->v += dv;
        if (motion->maxSpeed >= 0.f) {
            Vec3 dir = motion->v;
            float speed = dir.Normalize();
            if (speed > motion->maxSpeed) {
                motion->v = dir * motion->maxSpeed;
            } 
        }
		Vec3 dp = motion->v * dt;
		entity->_transform.Translate(dp);
	}
}
