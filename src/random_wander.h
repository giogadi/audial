#pragma once

#include "aabb.h"
#include "serial.h"
#include "new_entity.h"
#include "game_manager.h"

struct RandomWander {
    // serialized
    Aabb _bounds;
    float _speed = 1.f;

    // non-serialized
    Vec3 _currentVel;
    float _timeUntilNewPoint = -1.f;

    void Update(GameManager& g, float dt, ne::Entity* entity);

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool ImGui();
    void Init();
};
