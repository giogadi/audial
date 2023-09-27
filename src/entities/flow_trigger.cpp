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
    for (auto& pAction : _p._actionsOnExit) {
        pAction->Init(g);
    }

    _triggerVolumeEntities.clear();
    _triggerVolumeEntities.reserve(_p._triggerVolumeEditorIds.size());
    for (EditorId const& id : _p._triggerVolumeEditorIds) {
        ne::Entity* e = g._neEntityManager->FindEntityByEditorId(id);
        if (e) {
            _triggerVolumeEntities.push_back(e->_id);
        }
    }

    _isTriggering = false;
}

void FlowTriggerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("on_player_enter", _p._triggerOnPlayerEnter);
    pt.PutBool("use_trigger_volumes", _p._useTriggerVolumes);
    pt.PutInt("random_action_count", _p._randomActionCount);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p._actions);
    SeqAction::SaveActionsInChildNode(pt, "actions_on_exit", _p._actionsOnExit);
    serial::SaveVectorInChildNode(pt, "trigger_volume_entities", "editor_id", _p._triggerVolumeEditorIds);
}

void FlowTriggerEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    pt.TryGetBool("on_player_enter", &_p._triggerOnPlayerEnter);
    pt.TryGetBool("use_trigger_volumes", &_p._useTriggerVolumes);
    pt.TryGetInt("random_action_count", &_p._randomActionCount);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p._actions);
    SeqAction::LoadActionsFromChildNode(pt, "actions_on_exit", _p._actionsOnExit);
    serial::LoadVectorFromChildNode(pt, "trigger_volume_entities", _p._triggerVolumeEditorIds);
}

FlowTriggerEntity::ImGuiResult FlowTriggerEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;

    ImGui::Checkbox("On player enter", &_p._triggerOnPlayerEnter);
    ImGui::Checkbox("Use trigger volumes", &_p._useTriggerVolumes);
    ImGui::InputInt("Rand action count", &_p._randomActionCount);
    bool needsInit = SeqAction::ImGui("Actions", _p._actions);
    if (needsInit) {
        result = ImGuiResult::NeedsInit;
    }
    SeqAction::ImGui("Actions on Exit", _p._actionsOnExit);
    if (ImGui::CollapsingHeader("Trigger Volume Entities")) {
        ImGui::PushID("trigger_volumes");
        imgui_util::InputVector<EditorId>(_p._triggerVolumeEditorIds);
        ImGui::PopID();
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

void FlowTriggerEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode || !_p._triggerOnPlayerEnter) {
        return;
    }

    FlowPlayerEntity const* player = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
    if (player == nullptr) {
        return;
    }

    bool hasOverlap = false;
    if (_p._useTriggerVolumes) {
        for (ne::EntityId id : _triggerVolumeEntities) {
            ne::Entity* e = g._neEntityManager->GetEntity(id);
            if (e) {
                hasOverlap = geometry::DoAABBsOverlap(e->_transform, player->_transform, nullptr);
                if (hasOverlap) {
                    break;
                }
            }
        }
    } else {
        hasOverlap = geometry::DoAABBsOverlap(_transform, player->_transform, nullptr);
    }

    if (!_isTriggering && hasOverlap) {
        // Just entered the trigger!
        OnTrigger(g);
    }
    else if (_isTriggering && !hasOverlap) {
        for (auto const& pAction : _p._actionsOnExit) {
            pAction->Execute(g);
        }
    }
    _isTriggering = hasOverlap;
}
