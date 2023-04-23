#include "flow_trigger.h"

#include <sstream>

#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "entities/flow_player.h"
#include "geometry.h"

void FlowTriggerEntity::InitDerived(GameManager& g) {
    _actions.clear();
    _actions.reserve(_actionStrings.size());
    SeqAction::LoadInputs loadInputs;  // default
    for (std::string const& actionStr : _actionStrings) {
        std::stringstream ss(actionStr);
        std::unique_ptr<SeqAction> pAction = SeqAction::LoadAction(loadInputs, ss);
        if (pAction != nullptr) {
            pAction->Init(g);
            _actions.push_back(std::move(pAction));   
        }
    }

    _isTriggering = false;
}

void FlowTriggerEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveVectorInChildNode(pt, "actions", "action", _actionStrings);   
}

void FlowTriggerEntity::LoadDerived(serial::Ptree pt) {
    serial::LoadVectorFromChildNode(pt, "actions", _actionStrings);
}

FlowTriggerEntity::ImGuiResult FlowTriggerEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;   

    if (ImGui::CollapsingHeader("Actions")) {
        bool needsInit = imgui_util::InputVector(_actionStrings);
        if (needsInit) {
            result = ImGuiResult::NeedsInit;
        }
    }
    
    return result;
}

void FlowTriggerEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    FlowPlayerEntity const* player = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    if (player == nullptr) {
        return;
    }

    bool has_overlap = geometry::DoAABBsOverlap(_transform, player->_transform, nullptr);
    if (!_isTriggering && has_overlap) {
        // Just entered the trigger!
        for (auto const& pAction : _actions) {
            pAction->Execute(g);
        }
    }
    _isTriggering = has_overlap;
}
