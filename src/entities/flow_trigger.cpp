#include "flow_trigger.h"

#include <sstream>

#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "entities/flow_player.h"
#include "geometry.h"
#include "rng.h"
#include "entities/typing_enemy.h"

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

    if (_p._executeOnInit) {
        RunActions(g);
    }
}

void FlowTriggerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutDouble("trigger_delay_beat_time", _p._triggerDelayBeatTime);
    pt.PutBool("on_player_enter", _p._triggerOnPlayerEnter);
    pt.PutBool("execute_on_init", _p._executeOnInit);
    pt.PutBool("use_trigger_volumes", _p._useTriggerVolumes);
    pt.PutInt("random_action_count", _p._randomActionCount);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p._actions);
    SeqAction::SaveActionsInChildNode(pt, "actions_on_exit", _p._actionsOnExit);
    serial::SaveVectorInChildNode(pt, "trigger_volume_entities", "editor_id", _p._triggerVolumeEditorIds);
}

void FlowTriggerEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    pt.TryGetDouble("trigger_delay_beat_time", &_p._triggerDelayBeatTime);
    pt.TryGetBool("on_player_enter", &_p._triggerOnPlayerEnter);
    pt.TryGetBool("execute_on_init", &_p._executeOnInit);
    pt.TryGetBool("use_trigger_volumes", &_p._useTriggerVolumes);
    pt.TryGetInt("random_action_count", &_p._randomActionCount);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p._actions);
    SeqAction::LoadActionsFromChildNode(pt, "actions_on_exit", _p._actionsOnExit);
    serial::LoadVectorFromChildNode(pt, "trigger_volume_entities", _p._triggerVolumeEditorIds);
}

FlowTriggerEntity::ImGuiResult FlowTriggerEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;
    ImGui::InputDouble("Trigger delay (beats)", &_p._triggerDelayBeatTime);
    ImGui::Checkbox("On player enter", &_p._triggerOnPlayerEnter);
    ImGui::Checkbox("Execute on Init", &_p._executeOnInit);
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
    if (ImGui::Button("Assign enemies to me")) {
        std::vector<ne::Entity*> enemies;
        g._neEntityManager->GetEntitiesOfType(ne::EntityType::TypingEnemy, true, true, enemies);
        for (ne::Entity* b_e : enemies) {
            TypingEnemyEntity* e = static_cast<TypingEnemyEntity*>(b_e);
            if (e == nullptr) {
                continue;
            }
            Vec3 p = e->_transform.Pos();
            bool inside = geometry::IsPointInBoundingBox2d(p, _transform);
            if (inside) {
                e->_p._activeRegionEditorId = _editorId;
                e->Init(g);
            } else if (e->_p._activeRegionEditorId == _editorId) {
                e->_p._activeRegionEditorId = EditorId();
                e->Init(g);
            }
        }
    }
    return result;
}

void FlowTriggerEntity::OnTrigger(GameManager& g) {    
    if (_p._triggerDelayBeatTime >= 0.0) {
        double currentBeatTime = g._beatClock->GetBeatTimeFromEpoch();
        // TODO make denom an editable property
        double quantized = g._beatClock->GetNextBeatDenomTime(currentBeatTime, 1.0);
        _triggerTime = quantized + _p._triggerDelayBeatTime;
    } else {
        RunActions(g);
    }
}

void FlowTriggerEntity::RunActions(GameManager& g) {
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
    if (g._editMode) {
        return;
    }    

    if (_p._triggerOnPlayerEnter) {
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
        } else if (_isTriggering && !hasOverlap) {
            for (auto const& pAction : _p._actionsOnExit) {
                pAction->Execute(g);
            }
        }
        _isTriggering = hasOverlap;
    }

    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (_triggerTime >= 0.0 && beatTime >= _triggerTime) {
        RunActions(g);

        _triggerTime = -1.0;
    }
}
