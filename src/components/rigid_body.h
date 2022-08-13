#pragma once

#include <functional>

#include "component.h"
#include "enums/CollisionLayer.h"
#include "serial.h"
#include "matrix.h"

class CollisionManager;
class TransformComponent;

struct Aabb {
    Vec3 _min;
    Vec3 _max;

    void Save(ptree& pt) const {
        serial::SaveInNewChildOf(pt, "min", _min);
        serial::SaveInNewChildOf(pt, "max", _max);
    }
    void Load(ptree const& pt) {
        _min.Load(pt.get_child("min"));
        _max.Load(pt.get_child("max"));
    }
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
        : _localAabb(MakeCubeAabb(0.5f))
        , _velocity(0.f,0.f,0.f) {}
    virtual ~RigidBodyComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    virtual bool DrawImGui() override;

    // Make sure you don't set this callback to bind to a "this" pointer of some
    // random component. unsafe.
    typedef std::function<void(std::weak_ptr<RigidBodyComponent>)> OnHitCallback;

    void AddOnHitCallback(OnHitCallback f) {
        _onHitCallbacks.push_back(std::move(f));
    }

    void InvokeCallbacks(std::weak_ptr<RigidBodyComponent> other) {
        for (auto const& callback : _onHitCallbacks) {
            callback(other);
        }
    }

    void Save(ptree& pt) const override;
    void Load(ptree const& pt) override;

    // Serialized

    // If static, then collision manager won't move this component if there's a collision.
    bool _static = true;
    CollisionLayer _layer = CollisionLayer::Solid;
    Aabb _localAabb;  // Aabb assuming position = (0,0,0)

    Vec3 _velocity;
    std::weak_ptr<TransformComponent> _transform;
    CollisionManager* _collisionMgr;
    std::vector<OnHitCallback> _onHitCallbacks;

};