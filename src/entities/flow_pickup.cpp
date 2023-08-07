#include "flow_pickup.h"

#include <sstream>

#include "beat_clock.h"
#include "constants.h"
#include "seq_action.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"

namespace {
Vec4 constexpr kNoHitColor = Vec4(0.245098054f, 0.933390915f, 1.f, 1.f);
Vec4 constexpr kHitColor = Vec4(0.933390915f, 0.245098054f, 1.f, 1.f);
}

void FlowPickupEntity::InitDerived(GameManager& g) {
    for (auto const& pAction : _actions) {
        pAction->Init(g);
    }
    _hit = false;
    _modelColor = kNoHitColor;

    Quaternion q;
    q.SetFromAngleAxis(0.25f * kPi, Vec3(0.f, 1.f, 0.f));
    _transform.SetQuat(q);
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
    if (!_hit) {
        for (auto const& pAction : _actions) {
            pAction->Execute(g);
        }
    
        if (!g._editMode) {
            _modelColor = kHitColor;
            _hit = true;
        }
    }
}

void FlowPickupEntity::UpdateDerived(GameManager& g, float dt) {
    if (!g._editMode) {
        float speed;
        if (_hit) {
            speed = 4.f;
        } else {
            speed = 0.5f;
        }
        float angle = speed * dt * kPi;
        Quaternion q = _transform.Quat();
        Quaternion rot;
        rot.SetFromAngleAxis(angle, Vec3(0.f, 0.f, 1.f));
        q = rot * q;
        _transform.SetQuat(q);
    }  
}

void FlowPickupEntity::OnEditPick(GameManager& g) {
    OnHit(g);
}
