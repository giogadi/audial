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
    _p.key.Save(pt, "key"); 
    pt.PutDouble("quantize", _p.quantize);
    pt.PutFloat("angle", _p.angleDeg);
    pt.PutFloat("length", _p.length);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p.actions);
}

void GrabberEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    _p.key.Load(pt, "key");
    pt.TryGetDouble("quantize", &_p.quantize);
    pt.TryGetFloat("angle", &_p.angleDeg);
    pt.TryGetFloat("length", &_p.length);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p.actions);
}

ne::BaseEntity::ImGuiResult GrabberEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (_p.key.ImGui("Key")) {
        result = ImGuiResult::NeedsInit;
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
    if (SeqAction::ImGui("Actions", _p.actions)) {
        result = ImGuiResult::NeedsInit;
    }
    return result;
}

void GrabberEntity::InitDerived(GameManager& g) {
    _s = State();
    for (auto const& pAction : _p.actions) {
        pAction->Init(g);
    }
    _s.angleRad = _p.angleDeg * kDeg2Rad;
}

void GrabberEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    bool keyJustPressed = false;
    bool keyJustReleased = false;
    bool keyDown = false;
    bool quantizeReached = false;
    double quantizedBeatTime = -1.0;
    InputManager::Key k = InputManager::CharToKey(_p.key.c);
    keyJustPressed = g._inputManager->IsKeyPressedThisFrame(k);
    keyJustReleased = g._inputManager->IsKeyReleasedThisFrame(k);
    keyDown = g._inputManager->IsKeyPressed(k);
    if (keyJustPressed && _s.actionBeatTime < 0.0) {
        _s.actionBeatTime = g._beatClock->GetNextBeatDenomTime(beatTime, _p.quantize);
    }

    if (_s.actionBeatTime >= 0.0 && beatTime >= _s.actionBeatTime) {
        quantizeReached = true;
        quantizedBeatTime = _s.actionBeatTime;
        _s.actionBeatTime = -1.0;
    }

    if (quantizeReached) {
        for (auto const& pAction : _p.actions) {
            pAction->Execute(g);
        }
    }

    if (quantizeReached && _s.moveStartTime < 0.0) {
        _s.moveStartTime = quantizedBeatTime;
        _s.moveEndTime = quantizedBeatTime + 0.125;                
        _s.angleRad = Normalize2Pi(_s.angleRad);
        _s.moveStartAngleRad = _s.angleRad;
        _s.moveEndAngleRad = _s.moveStartAngleRad + (2 * kPi / 8);

        _s.resource = ne::EntityId();
    }
    if (_s.moveStartTime > 0.0) {
        Transform handTrans = GetGrabberHandTrans(_transform, _p.length, _s.angleRad);

        ResourceEntity* resInHand = g._neEntityManager->GetEntityAs<ResourceEntity>(_s.resource);
        if (resInHand == nullptr) {
            
            for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
                ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
                assert(r);
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
            Vec3 p = handTrans.Pos() + _s.handToResOffset;
            resInHand->_transform.SetPos(p);
        }

    }

}

void GrabberEntity::Draw(GameManager& g, float dt) {
    float const length = _p.length;

    Quaternion q;
    q.SetFromAngleAxis(_s.angleRad, Vec3(0.f, 1.f, 0.f));
    
    Transform armTrans;
    armTrans.SetQuat(q);
    armTrans.SetScale(Vec3(length, 0.25f, 0.25f));
    
    Vec3 dir = armTrans.GetXAxis();
    Vec3 p = _transform.Pos() + (0.5f * length) * dir;
    armTrans.SetPos(p);

    g._scene->DrawCube(armTrans.Mat4Scale(), _modelColor);
    
    p = _transform.Pos() + length * dir;
    Transform handTrans;
    handTrans.SetQuat(q);
    handTrans.SetScale(Vec3(1.f, 0.1f, 1.f));
    handTrans.SetPos(p);
    
    g._scene->DrawCube(handTrans.Mat4Scale(), _modelColor);


    {
        float constexpr kTextSize = 1.5f;
        Vec4 color(1.f, 1.f, 1.f, 1.f);
        char textBuf[] = "*";
        textBuf[0] = _p.key.c;
        // AHHHH ALLOCATION
        g._scene->DrawTextWorld(std::string(textBuf), _transform.Pos(), kTextSize, color);
    }

}

void GrabberEntity::OnEditPick(GameManager& g) {
}
