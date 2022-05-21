#pragma once

#include <vector>

#include "components/rigid_body.h"

class CollisionManager {
public:
    void AddBody(std::weak_ptr<RigidBodyComponent> body);
    void Update(float dt);

    std::vector<std::weak_ptr<RigidBodyComponent>> _bodies;
};