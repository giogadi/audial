#include "flow_wall.h"

#include <sstream>

#include "serial_vector_util.h"
#include "imgui_vector_util.h"
#include "math_util.h"
#include "renderer.h"
#include "editor.h"

void FlowWallEntity::InitDerived(GameManager& g) {
    _randomWander.Init();

    for (auto const& pAction : _hitActions) {
        pAction->Init(g);
    }

    _currentColor = _modelColor;
    _hp = _maxHp;
    _meshId = g._scene->LoadPolygon2d(_polygon);
    _canHit = _initCanHit;

    _children.clear();
    _children.reserve(_childrenEditorIds.size());
    for (EditorId const& id : _childrenEditorIds) {
        ne::BaseEntity* e = g._neEntityManager->FindEntityByEditorId(id, nullptr, "FlowWallEntity");
        if (e) {
            _children.push_back(e->_id);
        }        
    }
}

void FlowWallEntity::Update(GameManager& g, float dt) {
    // bool const editModeSelected = g._editMode && g._editor->IsEntitySelected(_id);
    
    switch (_moveMode) {
        case WaypointFollowerMode::Waypoints: {
            _wpFollower.Update(g, dt, this, _wpProps);
            break;
        }
        case WaypointFollowerMode::Random: {
            _randomWander.Update(g, dt, this);
            break;
        }
        case WaypointFollowerMode::Count: { assert(false); break; }
    }

    // Update rotation
    if (!g._editMode && _rotVel != 0.f) {
        float dAngle = _rotVel * dt;
        Quaternion qRot;
        qRot.SetFromAngleAxis(dAngle, Vec3(0.f, 1.f, 0.f));
        Quaternion newRot = qRot * _transform.Quat();
        _transform.SetQuat(newRot);
    }

    // At hp == maxHp, use modelColor. Fade to dark red as we get closer to 0.
    Vec4 colorAfterHp = _modelColor;
    if (_maxHp > 0) {
        // Vec4 constexpr kDyingColor(74.f / 255.f, 13.f / 255.f, 13.f / 255.f, 1.f);
        Vec4 const kDyingColor(1.f, 0.f, 0.f, _modelColor._w);
        float fracOfMaxHp = (float)_hp / (float)_maxHp;
        colorAfterHp = kDyingColor + fracOfMaxHp * (_modelColor - kDyingColor);
    }

    // Maybe update color from getting hit
    // turn to FadeColor on hit, then fade back to regular color
    if (_timeOfLastHit >= 0.0) {
        double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double constexpr kFadeTime = 0.25;
        double fadeFactor = (beatTime - _timeOfLastHit) / kFadeTime;
        fadeFactor = math_util::Clamp(fadeFactor, 0.0, 1.0);
        // Use same alpha as the model color on the entity
        
        Vec4 kFadeColor(0.f, 0.f, 0.f, _modelColor._w);
        if (fadeFactor == 1.0) {
            _timeOfLastHit = -1.0;
        }
        _currentColor = kFadeColor + fadeFactor * (colorAfterHp - kFadeColor);
    }
}

void FlowWallEntity::Draw(GameManager& g, float dt) 
{
    if (!_visible) {
        return;
    }

    Transform drawTransform = _transform;
    if (_timeOfLastHit >= 0.0) {
        double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double constexpr kEffectTime = 0.25;
        double effectPhase = (beatTime - _timeOfLastHit) / kEffectTime;
        effectPhase = math_util::Clamp(effectPhase, 0.0, 1.0);
        float factor = math_util::SmoothUpAndDown(effectPhase);
        float constexpr kMaxEffectAmount = 0.5f;
        Vec3 p = drawTransform.Pos();
        Vec3 dp = (factor * kMaxEffectAmount) * _lastHitDirection;
        p += dp;
        drawTransform.SetPos(p);
        for (ne::EntityId const& eId : _children) {
            if (ne::BaseEntity* child = g._neEntityManager->GetEntity(eId)) {
                Vec3 childP = child->_initTransform.Pos();
                childP += dp;
                child->_transform.SetPos(childP);
            }
        }
    }

    if (renderer::ModelInstance* model = g._scene->DrawMesh(_meshId)) {
        model->_transform = drawTransform.Mat4Scale();
        model->_color = _currentColor;
    } else if (_model != nullptr) {
        g._scene->DrawMesh(_model, drawTransform.Mat4Scale(), _currentColor);
    }
}


void FlowWallEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("hurt_on_hit", _hurtOnHit);
    pt.PutBool("can_hit", _initCanHit);
    pt.PutInt("hp", _maxHp);
    pt.PutString("move_mode", WaypointFollowerModeToString(_moveMode));
    if (_moveMode == WaypointFollowerMode::Waypoints) {
    } else if (_moveMode == WaypointFollowerMode::Random) {        
        _randomWander.Save(pt);
    }
    pt.PutFloat("rot_vel", _rotVel);
    serial::SaveVectorInChildNode(pt, "polygon", "point", _polygon);
    SeqAction::SaveActionsInChildNode(pt, "hit_actions", _hitActions);
    serial::SaveVectorInChildNode(pt, "children_editor_ids", "child", _childrenEditorIds);
}

void FlowWallEntity::LoadDerived(serial::Ptree pt) {
    _hurtOnHit = false;
    pt.TryGetBool("hurt_on_hit", &_hurtOnHit);
    _initCanHit = true;
    pt.TryGetBool("can_hit", &_initCanHit);
    pt.TryGetInt("hp", &_maxHp);
    std::string moveModeStr;
    bool changed = pt.TryGetString("move_mode", &moveModeStr);
    if (changed) {
        _moveMode = StringToWaypointFollowerMode(moveModeStr.c_str());
    }
    if (_moveMode == WaypointFollowerMode::Waypoints) {
    } else if (_moveMode == WaypointFollowerMode::Random) {
        _randomWander.Load(pt);
    }
    pt.TryGetFloat("rot_vel", &_rotVel);
    serial::LoadVectorFromChildNode(pt, "polygon", _polygon);
    SeqAction::LoadActionsFromChildNode(pt, "hit_actions", _hitActions);
    serial::LoadVectorFromChildNode(pt, "children_editor_ids", _childrenEditorIds);
}

FlowWallEntity::ImGuiResult FlowWallEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;

    ImGui::Checkbox("Hurt on hit", &_hurtOnHit);
    if (ImGui::Checkbox("Can hit", &_initCanHit)) {
        result = ImGuiResult::NeedsInit;
    }

    ImGui::InputInt("HP", &_maxHp);

    ImGui::InputFloat("Rot Vel", &_rotVel);

    WaypointFollowerModeImGui("Move mode", &_moveMode);
    
    if (_moveMode == WaypointFollowerMode::Waypoints) {
    } else if (_moveMode == WaypointFollowerMode::Random) {
        if (ImGui::CollapsingHeader("Random")) {
            _randomWander.ImGui();
        }
    }

    if (ImGui::TreeNode("Children")) {
        imgui_util::InputVector(_childrenEditorIds);
        ImGui::TreePop();
    }

    if (ImGui::CollapsingHeader("Polygon")) {
        imgui_util::InputVector(_polygon);        
    }

    if (SeqAction::ImGui("Hit actions", _hitActions)) {
        result = ImGuiResult::NeedsInit;
    }
    
    return result;
}

ne::BaseEntity::ImGuiResult FlowWallEntity::MultiImGui(GameManager& g, BaseEntity** entities, size_t entityCount) {
    ImGuiResult result = ImGuiResult::Done;

    if (ImGui::Checkbox("Hurt on hit", &_hurtOnHit)) {
        for (size_t ii = 0; ii < entityCount; ++ii) {
            FlowWallEntity* e = entities[ii]->As<FlowWallEntity>();
            e->_hurtOnHit = _hurtOnHit;
        }
    } 

    if (ImGui::Checkbox("Can hit", &_initCanHit)) {
        for (size_t ii = 0; ii < entityCount; ++ii) {
            FlowWallEntity* e = entities[ii]->As<FlowWallEntity>();
            e->_initCanHit = _initCanHit;
        }
    }

    if (ImGui::InputInt("HP", &_maxHp)) {
        for (size_t ii = 0; ii < entityCount; ++ii) {
            FlowWallEntity* e = entities[ii]->As<FlowWallEntity>();
            e->_maxHp = _maxHp;
        }
    }

    return result;
}

void FlowWallEntity::OnHit(GameManager& g, Vec3 const& hitDirection) {
    if (!g._editMode) {
        if (_maxHp > 0) {
            --_hp;
            if (_hp <= 0) {
                g._neEntityManager->TagForDestroy(_id);
            }
        }

        _timeOfLastHit = g._beatClock->GetBeatTimeFromEpoch();
        _lastHitDirection = hitDirection;
    }
    
    for (auto const& pAction : _hitActions) {
        pAction->Execute(g);
    }

}

void FlowWallEntity::OnEditPick(GameManager& g) {
    OnHit(g, Vec3());
}
