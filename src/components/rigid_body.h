#pragma once

#include <functional>

#include "component.h"

class CollisionManager;

struct Aabb {
    glm::vec3 _min;
    glm::vec3 _max;
};

inline Aabb MakeCubeAabb(float half_width) {
    Aabb a;
    a._min = glm::vec3(-half_width);
    a._max = glm::vec3(half_width);
    return a;
}

class RigidBodyComponent : public Component {
public:
    RigidBodyComponent(TransformComponent* t, CollisionManager* collisionMgr, Aabb const& localAabb);
    virtual ~RigidBodyComponent() {}
    virtual void Destroy() override;

    void SetOnHitCallback(std::function<void()> f) {
        _onHitCallback = f;
    }

    glm::vec3 _velocity;
    Aabb _localAabb;  // Aabb assuming position = (0,0,0)
    TransformComponent* _transform;
    CollisionManager* _collisionMgr;
    std::function<void()> _onHitCallback;
    // If static, then collision manager won't move this component if there's a collision.
    bool _static = true;
};