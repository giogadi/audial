#include "hit_counter.h"

#include <sstream>

#include "imgui/imgui.h"

#include "rigid_body.h"

bool HitCounterComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _t = e.FindComponentOfType<TransformComponent>();
    if (_t.expired()) {
        return false;
    }
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        return false;
    }
    auto pThisComp = e.FindComponentOfType<HitCounterComponent>();    
    _rb.lock()->AddOnHitCallback(std::bind(&HitCounterComponent::OnHit, pThisComp, std::placeholders::_1));
    return true;
}

void HitCounterComponent::OnHit(
    std::weak_ptr<HitCounterComponent> hitComp, std::weak_ptr<RigidBodyComponent> other) {
    hitComp.lock()->_wasHit = other.lock()->_layer == CollisionLayer::BodyAttack;
}

void HitCounterComponent::Update(float dt) {
    if (_wasHit) {
        --_hitsRemaining;
    }
    _wasHit = false;
}

bool HitCounterComponent::DrawImGui() {
    ImGui::InputScalar("Hits left", ImGuiDataType_S32, &_hitsRemaining, /*step=*/nullptr, /*???*/NULL, "%d");
    return false;
}