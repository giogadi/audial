#include "flow_pickup.h"

#include <sstream>

#include "beat_clock.h"
#include "constants.h"
#include "seq_action.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"

void FlowPickupEntity::InitDerived(GameManager& g) {
    for (auto const& pAction : _actions) {
        pAction->Init(g);
    }
}

void FlowPickupEntity::SaveDerived(serial::Ptree pt) const {
    SeqAction::SaveActionsInChildNode(pt, "actions", _actions);
}

void FlowPickupEntity::LoadDerived(serial::Ptree pt) {
    SeqAction::LoadActionsFromChildNode(pt, "actions", _actions);
}

ne::Entity::ImGuiResult FlowPickupEntity::ImGuiDerived(GameManager& g) {
    if (SeqAction::ImGui("Actions", _actions)) {
        return ne::Entity::ImGuiResult::NeedsInit;
    }
    return ne::Entity::ImGuiResult::Done;
}

void FlowPickupEntity::OnHit(GameManager& g) {
    for (auto const& pAction : _actions) {
        pAction->Execute(g);
    }
    
    if (!g._editMode) {
        g._neEntityManager->TagForDeactivate(_id);
    }
}

void FlowPickupEntity::Update(GameManager& g, float dt) {
    if (!g._editMode) {
        // Rotate about y-axis. half rotation every beat
        double beatTime = g._beatClock->GetBeatTimeFromEpoch();
        float angle = (float) beatTime * kPi;
        Quaternion q;
        q.SetFromAngleAxis(angle, Vec3(1.f, 1.f, 1.f).GetNormalized());
        _transform.SetQuat(q);
    }
    
    BaseEntity::Update(g, dt);
}

void FlowPickupEntity::OnEditPick(GameManager& g) {
    OnHit(g);
}
