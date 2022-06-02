#include "orbitable.h"

#include "imgui/imgui.h"

#include "rigid_body.h"

bool OrbitableComponent::ConnectComponents(Entity& e, GameManager& g) {
    _t = e.FindComponentOfType<TransformComponent>();
    if (_t.expired()) {
        return false;
    }
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        return false;
    }
    // std::weak_ptr<RigidBodyComponent> pRb = e.FindComponentOfType<RigidBodyComponent>();
    // if (pRb.expired()) {
    //     return false;
    // }
    // std::weak_ptr<BeepOnHitComponent> pBeep = e.FindComponentOfType<BeepOnHitComponent>();
    // RigidBodyComponent& rb = *pRb.lock();
    // rb.AddOnHitCallback(std::bind(&BeepOnHitComponent::OnHit, pBeep, std::placeholders::_1));
    _rb.lock()->_layer = CollisionLayer::None;
    return true;
}

// void OrbitableComponent::OnHit(
//     std::weak_ptr<BeepOnHitComponent> beepComp, std::weak_ptr<RigidBodyComponent> other) {
//     // Assumes neither object got destroyed. But at least we are able to handle this case later.
//     beepComp.lock()->_wasHit = other.lock()->_layer == CollisionLayer::BodyAttack;
// }

// void BeepOnHitComponent::Update(float dt) {
//     if (!_wasHit) {
//         return;
//     }
//     _wasHit = false;

//     PlayNotesOnNextDenom(/*denom=*/0.25);
// }

// bool BeepOnHitComponent::DrawImGui() {
//     ImGui::InputScalar("Channel", ImGuiDataType_S32, &_synthChannel, /*step=*/nullptr, /*???*/NULL, "%d");
//     char label[] = "Note XXX";
//     for (int i = 0; i < _midiNotes.size(); ++i) {        
//         sprintf(label, "Note %d", i);
//         ImGui::InputScalar(
//             label, ImGuiDataType_S32, &(_midiNotes[i]), /*step=*/nullptr, /*???*/NULL, "%d");
//     }
//     return false;
// }