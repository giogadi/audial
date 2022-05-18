#include "rigid_body.h"

#include "collisions.h"

void RigidBodyComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.DebugFindComponentOfType<TransformComponent>();
    _collisionMgr = g._collisionManager;
    _collisionMgr->AddBody(this);
}

void RigidBodyComponent::Destroy() {
    _collisionMgr->RemoveBody(this);
}