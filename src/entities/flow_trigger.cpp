#include "flow_trigger.h"

#include <sstream>

#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "entities/flow_player.h"
#include "geometry.h"
#include "rng.h"
#include "entities/typing_enemy.h"
#include "particle_mgr.h"

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

    _didInitTrigger = false;
}

void FlowTriggerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutDouble("trigger_delay_beat_time", _p._triggerDelayBeatTime);
    pt.PutDouble("quantize", _p._quantize);
    pt.PutBool("on_player_enter", _p._triggerOnPlayerEnter);
    pt.PutBool("execute_on_init", _p._executeOnInit);
    pt.PutBool("use_trigger_volumes", _p._useTriggerVolumes);
    pt.PutInt("random_action_count", _p._randomActionCount);
    pt.PutBool("shatter_on_trigger", _p._shatterOnTrigger);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p._actions);
    SeqAction::SaveActionsInChildNode(pt, "actions_on_exit", _p._actionsOnExit);
    serial::SaveVectorInChildNode(pt, "trigger_volume_entities", "editor_id", _p._triggerVolumeEditorIds);
}

void FlowTriggerEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    pt.TryGetDouble("trigger_delay_beat_time", &_p._triggerDelayBeatTime);
    pt.TryGetDouble("quantize", &_p._quantize);
    pt.TryGetBool("on_player_enter", &_p._triggerOnPlayerEnter);
    pt.TryGetBool("execute_on_init", &_p._executeOnInit);
    pt.TryGetBool("use_trigger_volumes", &_p._useTriggerVolumes);
    pt.TryGetInt("random_action_count", &_p._randomActionCount);
    pt.TryGetBool("shatter_on_trigger", &_p._shatterOnTrigger);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p._actions);
    SeqAction::LoadActionsFromChildNode(pt, "actions_on_exit", _p._actionsOnExit);
    serial::LoadVectorFromChildNode(pt, "trigger_volume_entities", _p._triggerVolumeEditorIds);
    if (pt.GetVersion() < 7) {
        // Backward compatibility. Before, we assumed that if there was a non-negative delay we would quantize to 1.
        if (_p._triggerDelayBeatTime >= 0.0 && _p._quantize <= 0.0) {
            _p._quantize = 1.0;
        }
    }
}

FlowTriggerEntity::ImGuiResult FlowTriggerEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;
    ImGui::InputDouble("Trigger delay (beats)", &_p._triggerDelayBeatTime);
    ImGui::InputDouble("Quantize", &_p._quantize);
    ImGui::Checkbox("On player enter", &_p._triggerOnPlayerEnter);
    ImGui::Checkbox("Execute on Init", &_p._executeOnInit);
    ImGui::Checkbox("Shatter on trigger", &_p._shatterOnTrigger);
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
    if (_p._quantize <= 0.0 && _p._triggerDelayBeatTime < 0.0) {
        RunActions(g);
        return;
    }

    double currentBeatTime = g._beatClock->GetBeatTimeFromEpoch();
    _triggerTime = g._beatClock->GetNextBeatDenomTime(currentBeatTime, _p._quantize);
    if (_p._triggerDelayBeatTime > 0.0) {
        _triggerTime += _p._triggerDelayBeatTime;
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
            int randNum = rng::GetIntGlobal(i, actionCount - 1);
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

    if (_p._shatterOnTrigger) {
        g._neEntityManager->TagForDeactivate(_id);

        // TODO: put this in a new kind of SeqAction
        Vec3 boxP = _transform.Pos();
        // TODO: Assumes AABB
        Vec3 minP = boxP - _transform.Scale()*0.5f;
        Vec3 maxP = boxP + _transform.Scale()*0.5f;
        for (int ii = 0; ii < 20; ++ii) {
            Vec3 p;
            p._x = rng::GetFloatGlobal(minP._x, maxP._x);
            p._y = rng::GetFloatGlobal(minP._y, maxP._y);
            p._z = rng::GetFloatGlobal(minP._z, maxP._z);
            Vec3 dir = p - boxP;
            dir.Normalize(); 
            
            Particle* particle = g._particleMgr->SpawnParticle(); 
            particle->t.SetPos(p);
            particle->t.SetScale(Vec3(0.15f, 0.15f, 0.15f));
            particle->v = 2.f * dir;
            particle->rotV = 4.f * dir;
            particle->color = _modelColor;
            particle->alphaV = -0.5f;
            particle->timeLeft = 0.5f;
            particle->shape = ParticleShape::Triangle;
        }
    }
}

void FlowTriggerEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    FlowPlayerEntity const* player = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));

    if (_p._executeOnInit && !_didInitTrigger) {
        if (_flowSectionId < 0 || player->_s._currentSectionId == _flowSectionId) {
            RunActions(g);
            _didInitTrigger = true;
        }        
    }

    if (_p._triggerOnPlayerEnter) {
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
