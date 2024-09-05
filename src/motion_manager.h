#pragma once

#include "new_entity_id.h"
#include "matrix.h"
#include "game_manager.h"

struct Motion {
	ne::EntityId entityId;

	Vec3 v;
	float timeLeft = 0.f;
};

struct MotionManager {
	static int constexpr kMaxMotions = 1024;

	Motion _motions[kMaxMotions];
	int _motionCount = 0;

	Motion* AddMotion(ne::EntityId entityId);

	void Update(float dt, GameManager& g);
};