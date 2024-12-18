#include "entities/grabber.h"

#include <imgui/imgui.h>

#include "game_manager.h"
#include "renderer.h"
#include "math_util.h"
#include "entities/resource.h"
#include "geometry.h"

namespace {
float Normalize2Pi(float angleRad) {
    return angleRad - (2*kPi) * std::floor(angleRad / (2*kPi));
}

Transform GetGrabberHandTrans(Transform const& grabberTrans, float grabberLength, float grabberAngleRad) {
    Transform handTrans;

    Quaternion q;
    q.SetFromAngleAxis(grabberAngleRad, Vec3(0.f, 1.f, 0.f));

    handTrans.SetQuat(q); 
    handTrans.SetScale(Vec3(1.f, 1.f, 1.f));
    Vec3 dir = handTrans.GetXAxis();
    Vec3 handP = grabberTrans.Pos() + grabberLength * dir;
    handTrans.SetPos(handP);

    return handTrans;
}
}


void GrabberEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("has_plus_rot", _p.hasPlusRot);
    serial::SaveInNewChildOf(pt, "plus_rot_key", _plusRotKey);
    pt.PutBool("has_neg_rot", _p.hasNegRot);
    serial::SaveInNewChildOf(pt, "neg_rot_key", _negRotKey);
    pt.PutDouble("quantize", _p.quantize);
    pt.PutFloat("angle", _p.angleDeg);
    pt.PutFloat("length", _p.length);
    pt.PutFloat("height", _p.height);
    SeqAction::SaveActionsInChildNode(pt, "plus_rot_actions", _p.plusRotActions);
    SeqAction::SaveActionsInChildNode(pt, "neg_rot_actions", _p.negRotActions);
}

void GrabberEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    pt.TryGetBool("has_plus_rot", &_p.hasPlusRot);
    serial::LoadFromChildOf(pt, "plus_rot_key", _plusRotKey);
    pt.TryGetBool("has_neg_rot", &_p.hasNegRot);
    serial::LoadFromChildOf(pt, "neg_rot_key", _negRotKey);
    pt.TryGetDouble("quantize", &_p.quantize);
    pt.TryGetFloat("angle", &_p.angleDeg);
    pt.TryGetFloat("length", &_p.length);
    pt.TryGetFloat("height", &_p.height);
    if (pt.GetVersion() < 6) {
        SeqAction::LoadActionsFromChildNode(pt, "actions", _p.plusRotActions);
    } else {
        SeqAction::LoadActionsFromChildNode(pt, "plus_rot_actions", _p.plusRotActions);
        SeqAction::LoadActionsFromChildNode(pt, "neg_rot_actions", _p.negRotActions);
    }
}

ne::BaseEntity::ImGuiResult GrabberEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (ImGui::TreeNode("+rot key")) {
        ImGui::Checkbox("Has key", &_p.hasPlusRot);
        _plusRotKey.ImGui();
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("-rot key")) {
        ImGui::Checkbox("Has key", &_p.hasNegRot);
        _negRotKey.ImGui();
        ImGui::TreePop();
    }

    
    if (ImGui::InputDouble("Quantize", &_p.quantize)) {
        result = ImGuiResult::NeedsInit;
    }
    if (ImGui::SliderFloat("Angle", &_p.angleDeg, -180.f, 180.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        result = ImGuiResult::NeedsInit;
    }
    if (ImGui::SliderFloat("Length", &_p.length, 1.f, 10.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        result = ImGuiResult::NeedsInit;
    }
    if (ImGui::SliderFloat("Height", &_p.height, 0.f, 10.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        result = ImGuiResult::NeedsInit;
    }

    if (SeqAction::ImGui("+Rot Actions", _p.plusRotActions)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("-Rot Actions", _p.negRotActions)) {
        result = ImGuiResult::NeedsInit;
    }
    return result;
}

void GrabberEntity::InitDerived(GameManager& g) {
    _s = State();
    _plusRotKey.Init();
    _negRotKey.Init();
    for (auto const& pAction : _p.plusRotActions) {
        pAction->Init(g);
    }
    for (auto const& pAction : _p.negRotActions) {
        pAction->Init(g);
    }
    _s.angleRad = _p.angleDeg * kDeg2Rad;
}

void GrabberEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    if (_p.hasPlusRot) {
        _plusRotKey.Update(g, dt);
    }
    if (_p.hasNegRot) {
        _negRotKey.Update(g, dt);
    }
    
    bool quantizeReached = false;
    double quantizedBeatTime = -1.0; 

    if (_s.actionBeatTime < 0.0) {
        bool actionStarted = false;
        if (_plusRotKey._s.keyJustPressed) {
            actionStarted = true;
            _s.posAction = true;
        } else if (_negRotKey._s.keyJustPressed) {
            actionStarted = true;
            _s.posAction = false;
        }
        if (actionStarted) {
            _s.actionBeatTime = g._beatClock->GetNextBeatDenomTime(beatTime, _p.quantize);
            assert(_s.actionBeatTime >= 0.0);
        }
    }
 
    if (_s.actionBeatTime >= 0.0 && beatTime >= _s.actionBeatTime) {
        quantizeReached = true;
        quantizedBeatTime = _s.actionBeatTime;
        _s.actionBeatTime = -1.0;
    }

    if (quantizeReached) {
        auto const& actions = _s.posAction ? _p.plusRotActions : _p.negRotActions;
        for (auto const& pAction : actions) {
            pAction->Execute(g);
        }
    }

    if (quantizeReached && _s.moveStartTime < 0.0) {
        assert(quantizedBeatTime >= 0.0);
        _s.moveStartTime = quantizedBeatTime;
        _s.moveEndTime = quantizedBeatTime + 0.125;                
        _s.angleRad = Normalize2Pi(_s.angleRad);
        _s.moveStartAngleRad = _s.angleRad;
        _s.moveEndAngleRad = _s.moveStartAngleRad + (_s.posAction ? 1.f : -1.f) * (2 * kPi / 8);

        _s.resource = ne::EntityId();
    }
    if (_s.moveStartTime >= 0.0) {
        Transform handTrans = GetGrabberHandTrans(_transform, _p.length, _s.angleRad);

        ResourceEntity* resInHand = g._neEntityManager->GetEntityAs<ResourceEntity>(_s.resource);
        if (resInHand == nullptr) {
            
            for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
                ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
                assert(r);
                if (r->_s.destroyed) {
                    continue;
                }
                bool inHand = geometry::IsPointInBoundingBox2d(r->_transform.Pos(), handTrans);
                if (inHand) {
                    _s.resource = r->_id;
                    resInHand = r;
                    _s.handToResOffset = r->_transform.Pos() - handTrans.Pos();
                    break;
                }
            }
        }


        float factor = (float) math_util::InverseLerp(_s.moveStartTime, _s.moveEndTime, beatTime);   
        factor = math_util::SmoothStep(factor);
        _s.angleRad = math_util::Lerp(_s.moveStartAngleRad, _s.moveEndAngleRad, factor);
        if (beatTime >= _s.moveEndTime) {
            _s.moveStartTime = -1.0;
            _s.moveEndTime = -1.0;
        }

        handTrans = GetGrabberHandTrans(_transform, _p.length, _s.angleRad); 

        if (resInHand) {
            Vec3 p = handTrans.Pos(); // + _s.handToResOffset;
            p._y += _s.handToResOffset._y;
            resInHand->_transform.SetPos(p);
        }

    }

}

void GrabberEntity::Draw(GameManager& g, float dt) {
    Transform cubeCtrToBtm;
    cubeCtrToBtm.SetPos(Vec3(0.f, 0.5f, 0.f));
    Mat4 toBtmM = cubeCtrToBtm.Mat4NoScale();

    float constexpr kPlatHeight = 0.1f;
    Vec3 platRenderScale = _transform.Scale();
    platRenderScale._y = kPlatHeight;
    Transform platRenderT;
    platRenderT = _transform;
    platRenderT.SetScale(platRenderScale);
    Mat4 platRenderM = platRenderT.Mat4Scale() * toBtmM;
    g._scene->DrawCube(platRenderM, _modelColor);

    Transform btnTrans = _transform;
    btnTrans.Translate(Vec3(0.f, kPlatHeight, 0.f));
    if (_p.hasPlusRot) {
        _plusRotKey.Draw(g, btnTrans);
    }
    if (_p.hasNegRot) {
        _negRotKey.Draw(g, btnTrans);
    }


    float const length = _p.length;

    Quaternion q;
    q.SetFromAngleAxis(_s.angleRad, Vec3(0.f, 1.f, 0.f));
    
    Transform armTrans;
    armTrans.SetQuat(q);
    float constexpr kArmScaleY = 0.1f;
    armTrans.SetScale(Vec3(length, kArmScaleY, 0.25f));
    
    Vec3 dir = armTrans.GetXAxis();
    Vec3 p = _transform.Pos() + (0.5f * length) * dir;
    p._y += 0.5f * kArmScaleY;
    p._y += _p.height;
    armTrans.SetPos(p);

    g._scene->DrawCube(armTrans.Mat4Scale(), _modelColor);
    
    p = _transform.Pos() + length * dir;
    p._y += 0.5f * kArmScaleY;
    p._y += _p.height;
    Transform handTrans;
    handTrans.SetQuat(q);
    handTrans.SetScale(Vec3(1.f, kArmScaleY, 1.f));
    handTrans.SetPos(p);
    
    g._scene->DrawCube(handTrans.Mat4Scale(), _modelColor); 
}

void GrabberEntity::OnEditPick(GameManager& g) {
}
