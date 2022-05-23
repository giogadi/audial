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

    // Make sure you don't set this callback to bind to a "this" pointer of some
    // random component. unsafe.
    typedef std::function<void(std::weak_ptr<RigidBodyComponent>)> OnHitCallback;

    void SetOnHitCallback(OnHitCallback f) {
        _onHitCallback = f;
    }

    Vec3 _velocity;
    Aabb _localAabb;  // Aabb assuming position = (0,0,0)
    std::weak_ptr<TransformComponent> _transform;
    CollisionManager* _collisionMgr;
    OnHitCallback _onHitCallback;
    // If static, then collision manager won't move this component if there's a collision.
    bool _static = true;
    CollisionLayer _layer = CollisionLayer::Solid;
};