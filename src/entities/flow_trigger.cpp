#include "flow_trigger.h"

#include <sstream>

#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "entities/flow_player.h"
#include "geometry.h"
#include "rng.h"

void FlowTriggerEntity::InitDerived(GameManager& g) {
    for (auto& pAction : _p._actions) {
        pAction->Init(g);
    }

    _isTriggering = false;
}

void FlowTriggerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("on_player_enter", _p._triggerOnPlayerEnter);
    pt.PutInt("random_action_count", _p._randomActionCount);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p._actions);
}

void FlowTriggerEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    pt.TryGetBool("on_player_enter", &_p._triggerOnPlayerEnter);
    pt.TryGetInt("random_action_count", &_p._randomActionCount);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p._actions);
}

FlowTriggerEntity::ImGuiResult FlowTriggerEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;

    ImGui::Checkbox("On player enter", &_p._triggerOnPlayerEnter);
    ImGui::InputInt("Rand action count", &_p._randomActionCount);
    bool needsInit = SeqAction::ImGui("Actions", _p._actions);
    if (needsInit) {
        result = ImGuiResult::NeedsInit;
    }   
    
    return result;
}

void FlowTriggerEntity::OnTrigger(GameManager& g) {
    if (_p._randomActionCount > 0) {
        int const actionCount = _p._actions.size();
        _randomDrawList.resize(actionCount);
        for (int i = 0; i < actionCount; ++i) {
            _randomDrawList[i] = i;
        }
        int const numActionsToExecute = std::min(_p._randomActionCount, actionCount);
        for (int i = 0; i < numActionsToExecute; ++i) {
            int randNum = rng::GetInt(i, actionCount - 1);
            int actionIx = _randomDrawList[randNum];
            _p._actions[actionIx]->Execute(g);
            if (i != actionIx) {
                std::swap(_randomDrawList[i], _randomDrawList[randNum]);
            }
        }
    } else {
        for (auto const& pAction : _p._actions) {
            pAction->Execute(g);
        }
    }
}

void FlowTriggerEntity::Update(GameManager& g, float dt) {
    if (g._editMode || !_p._triggerOnPlayerEnter) {
        return;
    }

    FlowPlayerEntity const* player = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    if (player == nullptr) {
        return;
    }

    bool has_overlap = geometry::DoAABBsOverlap(_transform, player->_transform, nullptr);
    if (!_isTriggering && has_overlap) {
        // Just entered the trigger!
        OnTrigger(g);
    }
    _isTriggering = has_overlap;
}
