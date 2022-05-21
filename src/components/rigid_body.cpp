#include "rigid_body.h"

#include "collisions.h"

void RigidBodyComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    _collisionMgr = g._collisionManager;
    _collisionMgr->AddBody(e.FindComponentOfType<RigidBodyComponent>());
}