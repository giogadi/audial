#include "flow_wall.h"

#include <sstream>

#include "serial_vector_util.h"
#include "imgui_vector_util.h"

void FlowWallEntity::InitDerived(GameManager& g) {
    _wpFollower.Init(_transform.Pos());
    
    _hitActions.reserve(_hitActionStrings.size());
    SeqAction::LoadInputs loadInputs;  // default
    for (std::string const& actionStr : _hitActionStrings) {
        std::stringstream ss(actionStr);
        std::unique_ptr<SeqAction> pAction = SeqAction::LoadAction(loadInputs, ss);
        if (pAction != nullptr) {
            pAction->InitBase(g);
            _hitActions.push_back(std::move(pAction));   
        }
    }
}

void FlowWallEntity::Update(GameManager& g, float dt) {

    if (!g._editMode) {
        Vec3 newPos;
        bool updatePos = _wpFollower.Update(g, dt, &newPos);
        if (updatePos) {
            _transform.SetPos(newPos);
        }
    }

    BaseEntity::Update(g, dt);
}

void FlowWallEntity::SaveDerived(serial::Ptree pt) const {
    _wpFollower.Save(pt);

    serial::SaveVectorInChildNode(pt, "hit_actions", "action", _hitActionStrings);   
}

void FlowWallEntity::LoadDerived(serial::Ptree pt) {
    _wpFollower.Load(pt);

    serial::LoadVectorFromChildNode(pt, "hit_actions", _hitActionStrings);
}

FlowWallEntity::ImGuiResult FlowWallEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;
    
    if (ImGui::CollapsingHeader("Waypoints")) {
        _wpFollower.ImGui();
    }

    if (ImGui::CollapsingHeader("Hit actions")) {
        bool needsInit = imgui_util::InputVector(_hitActionStrings);
        if (needsInit) {
            result = ImGuiResult::NeedsInit;
        }
    }
    
    return result;
}

void FlowWallEntity::OnHit(GameManager& g) {
    for (auto const& pAction : _hitActions) {
        pAction->ExecuteBase(g);
    }

}

void FlowWallEntity::OnEditPick(GameManager& g) {
    OnHit(g);
}
