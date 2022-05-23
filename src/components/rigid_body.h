#pragma once

#include <functional>

#include "component.h"
#include "enums/CollisionLayer.h"

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
    virtual ComponentType Type() const override { return ComponentType::RigidBody; }
    RigidBodyComponent()
        : _velocity(0.f,0.f,0.f) {}
    virtual ~RigidBodyComponent() {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;

    void SetOnHitCallback(std::function<void(std::weak_ptr<RigidBodyComponent>)> f) {
        _onHitCallback = f;
    }

    Vec3 _velocity;
    Aabb _localAabb;  // Aabb assuming position = (0,0,0)
    std::weak_ptr<TransformComponent> _transform;
    CollisionManager* _collisionMgr;
    std::function<void(std::weak_ptr<RigidBodyComponent>)> _onHitCallback;
    // If static, then collision manager won't move this component if there's a collision.
    bool _static = true;
    CollisionLayer _layer = CollisionLayer::Solid;
};