#include "rigid_body.h"

#include "collisions.h"

RigidBodyComponent::RigidBodyComponent(TransformComponent* t, CollisionManager* collisionMgr, Aabb const& localAabb)
    : _velocity(0.f)
    , _transform(t)
    , _collisionMgr(collisionMgr)
    , _localAabb(localAabb) {
    _collisionMgr->AddBody(this);
}

void RigidBodyComponent::Destroy() {
    _collisionMgr->RemoveBody(this);
}