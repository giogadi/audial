#pragma once

#include <vector>

#include "components/rigid_body.h"

class CollisionManager {
public:
    void AddBody(RigidBodyComponent* body);
    void RemoveBody(RigidBodyComponent* body);
    void Update(float dt);

    std::vector<RigidBodyComponent*> _bodies;
};