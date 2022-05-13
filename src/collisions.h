#pragma once

#include <vector>

#include "components/rigid_body.h"

enum class CollisionLayer : int {
    Solid,
    BodyAttack
};

class CollisionManager {
public:
    void AddBody(RigidBodyComponent* body);
    void RemoveBody(RigidBodyComponent* body);
    void Update(float dt);

    std::vector<RigidBodyComponent*> _bodies;
};