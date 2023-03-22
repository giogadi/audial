#include "flow_pickup.h"

#include <sstream>

#include "beat_clock.h"
#include "constants.h"
#include "seq_action.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"

void FlowPickupEntity::InitDerived(GameManager& g) {
    _actions.clear();
    _actions.reserve(_actionStrings.size());    
    SeqAction::LoadInputs loadInputs;  // default
    for (std::string const& actionStr : _actionStrings) {
        std::stringstream ss(actionStr);
        std::unique_ptr<SeqAction> pAction = SeqAction::LoadAction(loadInputs, ss);
        if (pAction != nullptr) {
            pAction->InitBase(g);
            _actions.push_back(std::move(pAction));   
        }
    }
}

void FlowPickupEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveVectorInChildNode(pt, "actions", "action", _actionStrings);   
}

void FlowPickupEntity::LoadDerived(serial::Ptree pt) {
    serial::LoadVectorFromChildNode(pt, "actions", _actionStrings);
}

ne::Entity::ImGuiResult FlowPickupEntity::ImGuiDerived(GameManager& g) {
    bool needsInit = false;
    if (ImGui::CollapsingHeader("Actions")) {
        needsInit = imgui_util::InputVector(_actionStrings);  
    }
    return needsInit ? ne::Entity::ImGuiResult::NeedsInit : ne::Entity::ImGuiResult::Done;
}

void FlowPickupEntity::OnHit(GameManager& g) {
    for (auto const& pAction : _actions) {
        pAction->ExecuteBase(g);
    }
    
    if (!g._editMode) {
        g._neEntityManager->TagForDestroy(_id);
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
