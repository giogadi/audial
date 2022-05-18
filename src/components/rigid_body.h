#pragma once

#include <functional>

#include "component.h"

class CollisionManager;

struct Aabb {
    Vec3 _min;
    Vec3 _max;
};

inline Aabb MakeCubeAabb(float half_width) {
    Aabb a;
    a._min = Vec3(-half_width, -half_width, -half_width);
    a._max = Vec3(half_width, half_width, half_width);
    return a;
}

class RigidBodyComponent : public Component {
public:
    virtual ComponentType Type() override { return ComponentType::RigidBody; }
    RigidBodyComponent()
        : _velocity(0.f,0.f,0.f) {}
    virtual ~RigidBodyComponent() {}
    virtual void Destroy() override;
    virtual void ConnectComponents(Entity& e, GameManager& g) override;

    void SetOnHitCallback(std::function<void()> f) {
        _onHitCallback = f;
    }

    Vec3 _velocity;
    Aabb _localAabb;  // Aabb assuming position = (0,0,0)
    TransformComponent* _transform;
    CollisionManager* _collisionMgr;
    std::function<void()> _onHitCallback;
    // If static, then collision manager won't move this component if there's a collision.
    bool _static = true;
};