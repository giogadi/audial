#pragma once

#include "new_entity_id.h"
#include "matrix.h"
#include "game_manager.h"
#include "editor_id.h"

namespace ne {
struct BaseEntity;
}

struct MotionSpec {
	bool updatePos = true;
	Vec3 v0;
	Vec3 a;
	float maxSpeed = -1.f;

	bool doColor = false;	
	Vec4 endColor;

	bool doScale = false;
	Vec3 scaleToApply = Vec3(1.f, 1.f, 1.f);

	float totalTime = 0.f;
	
	void Save(serial::Ptree pt) const;
	void Load(serial::Ptree pt);
	bool ImGui();
};

struct Motion {
	MotionSpec spec;
	ne::EntityId entityId;	
	float timeLeft = 0.f;
	Vec3 v;
	Vec4 startColor;
	Vec3 startScale;
	Vec3 endScale;
};

struct MotionManager {	
	Motion *_motions = nullptr;
	int _motionCount = 0;

	Motion* AddMotion(MotionSpec const &spec, ne::BaseEntity *entity);

	void Init();
	void Update(float dt, GameManager& g);
	void Destroy();
};
