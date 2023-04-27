#include "flow_trigger.h"

#include <sstream>

#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "entities/flow_player.h"
#include "geometry.h"

void FlowTriggerEntity::InitDerived(GameManager& g) {
    for (auto& pAction : _actions) {
        pAction->Init(g);
    }

    _isTriggering = false;
}

void FlowTriggerEntity::SaveDerived(serial::Ptree pt) const {
    SeqAction::SaveActionsInChildNode(pt, "actions", _actions);
}

void FlowTriggerEntity::LoadDerived(serial::Ptree pt) {
    SeqAction::LoadActionsFromChildNode(pt, "actions", _actions);
}

FlowTriggerEntity::ImGuiResult FlowTriggerEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;   

    bool needsInit = SeqAction::ImGui("Actions", _actions);
    if (needsInit) {
        result = ImGuiResult::NeedsInit;
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
