#include "orbitable.h"

#include "imgui/imgui.h"

#include "rigid_body.h"

bool OrbitableComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _t = e.FindComponentOfType<TransformComponent>();
    if (_t.expired()) {
        return false;
    }
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        return false;
    }
    _rb.lock()->_layer = CollisionLayer::None;
    return true;
}