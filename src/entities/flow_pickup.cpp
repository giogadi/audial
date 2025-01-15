#include "flow_pickup.h"

#include <sstream>

#include "beat_clock.h"
#include "constants.h"
#include "seq_action.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"
#include "math_util.h"
#include "imgui_util.h"
#include "renderer.h"
#include "features.h"

namespace {
//Vec4 constexpr kNoHitColor = Vec4(0.245098054f, 0.933390915f, 1.f, 1.f);
Vec4 constexpr kHitColor = Vec4(0.933390915f, 0.245098054f, 1.f, 1.f);
}

void FlowPickupEntity::InitDerived(GameManager& g) {
    for (auto const& pAction : _onHitActions) {
        pAction->Init(g);
    }
    for (auto const& pAction : _onDeathActions) {
        pAction->Init(g);
    }
    _hit = false;
    _beatTimeOfDeath = -1.0;

    Quaternion q;
    q.SetFromAngleAxis(0.25f * kPi, Vec3(0.f, 1.f, 0.f));
    _transform.SetQuat(q);
}

void FlowPickupEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("kop", _killOnPickup);
    serial::SaveInNewChildOf(pt, "hc", _hitColor);
    SeqAction::SaveActionsInChildNode(pt, "actions", _onHitActions);
    SeqAction::SaveActionsInChildNode(pt, "deathActions", _onDeathActions);

}

void FlowPickupEntity::LoadDerived(serial::Ptree pt) {
    SeqAction::LoadActionsFromChildNode(pt, "actions", _onHitActions);
    SeqAction::LoadActionsFromChildNode(pt, "deathActions", _onDeathActions);

    _killOnPickup = false;
    pt.TryGetBool("kop", &_killOnPickup);
    _hitColor = kHitColor;
    serial::LoadFromChildOf(pt, "hc", _hitColor);
}

ne::Entity::ImGuiResult FlowPickupEntity::ImGuiDerived(GameManager& g) {
    ImGui::Checkbox("Kill on pickup", &_killOnPickup);
    imgui_util::ColorEdit4("Hit color", &_hitColor);
    if (SeqAction::ImGui("Hit actions", _onHitActions)) {
        return ne::Entity::ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("Death actions", _onDeathActions)) {
        return ne::Entity::ImGuiResult::NeedsInit;
    }
    return ne::Entity::ImGuiResult::Done;
}

void FlowPickupEntity::OnHit(GameManager& g) {
    bool doActions = false;
    if (g._editMode) {
        doActions = true;
    } 
    if (!_hit) {
        if (_killOnPickup) {
            _beatTimeOfDeath = g._beatClock->GetBeatTimeFromEpoch();
        }
        //if (!_killOnPickup) {
           doActions = true; 
        //}
    
        //if (!g._editMode) {
            _hit = true;
       // }
    }
    if (doActions) {
        for (auto const& pAction : _onHitActions) {
            pAction->Execute(g);
        }
    }
}

void FlowPickupEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }
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

    if (_beatTimeOfDeath >= 0.0) {
        double constexpr kDeathDuration = 1.0;
        double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double t = beatTime - _beatTimeOfDeath;
        double x = math_util::InverseLerp(0.0, kDeathDuration, t);

        Vec3 minScale = Vec3(1.f, 1.f, 1.f) * 0.05f;
        Vec3 scale = math_util::Vec3Lerp((float)x, _initTransform.Scale(), minScale);
        _transform.SetScale(scale);

       if (t >= kDeathDuration) {
           g._neEntityManager->TagForDeactivate(_id);
           for (auto const &pAction : _onDeathActions) { 
               pAction->Execute(g);
           }
       } 
    }
}

void FlowPickupEntity::Draw(GameManager &g, float dt) {
    Vec4 currentColor = _hit ? _hitColor : _modelColor;
    if (_model != nullptr) {
        Mat4 const& mat = _transform.Mat4Scale();
        renderer::ModelInstance* m = g._scene->DrawTexturedMesh(_model, _textureId);
        m->_transform = mat;
        m->_color = currentColor;        
#if NEW_LIGHTS
        m->_lightFactor = 0.f;
        renderer::Light *light = g._scene->DrawLight();
        light->_p = _transform.Pos();
        light->_color = currentColor.GetXYZ();
        light->_ambient = 0.f;
        light->_diffuse = 1.f;
        light->_specular = 0.f;
        light->_range = 20.f;
        light->_isDirectional = false;
#endif
    } else if (g._editMode) {
        // ???
        Mat4 const& mat = _transform.Mat4Scale();
        Vec4 constexpr bbColor(0.5f, 0.5f, 0.5f, 1.f);
        g._scene->DrawBoundingBox(mat, bbColor);
    }
}

void FlowPickupEntity::OnEditPick(GameManager& g) {
    OnHit(g);
}
