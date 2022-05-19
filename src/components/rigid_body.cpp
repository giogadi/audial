#include "rigid_body.h"

#include <unordered_map>
#include <string>

#include "collisions.h"

namespace {

char const* gCollisionLayerStrings[] = {
#   define X(a) #a,
    M_COLLISION_LAYERS
#   undef X
};

std::unordered_map<std::string, CollisionLayer> const gStringToCollisionLayer = {
#   define X(name) {#name, CollisionLayer::name},
    M_COLLISION_LAYERS
#   undef X
};

} // end namespace

char const* CollisionLayerToString(CollisionLayer c) {
    return gCollisionLayerStrings[static_cast<int>(c)];
}

CollisionLayer StringToCollisionLayer(char const* c) {
    return gStringToCollisionLayer.at(c);
}

void RigidBodyComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.DebugFindComponentOfType<TransformComponent>();
    _collisionMgr = g._collisionManager;
    _collisionMgr->AddBody(this);
}

void RigidBodyComponent::Destroy() {
    _collisionMgr->RemoveBody(this);
}