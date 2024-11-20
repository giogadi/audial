#include "mech.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "input_manager.h"
#include "renderer.h"
#include "entity_manager.h"
#include "geometry.h"
#include "beat_clock.h"
#include "math_util.h"
#include "entities/resource.h"

void MechEntity::InitTypeSpecificProps() {
    switch (_p.type) {
    case MechType::Pusher:
        _p.pusher = PusherProps();
        _p.pusher.angleDeg = 0.f;
        break;
    case MechType::Count: break;
    }
}

void MechEntity::InitTypeSpecificState() {
    switch (_p.type) {
    case MechType::Pusher:
        _s.pusher = PusherState();
        break;
   case MechType::Count: break;
    }
}

void MechEntity::SaveDerived(serial::Ptree pt) const {
    serial::PutEnum(pt, "mech_type", _p.type);
    pt.PutChar("key", _p.key);
    pt.PutDouble("quantize", _p.quantize);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p.actions);
    SeqAction::SaveActionsInChildNode(pt, "stop_actions", _p.stopActions);

    switch (_p.type) {
    case MechType::Pusher:
        pt.PutFloat("pusher_angle", _p.pusher.angleDeg);
        break;
    case MechType::Count: break;
    }
}

void MechEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();    
    serial::TryGetEnum(pt, "mech_type", _p.type);
    
    InitTypeSpecificProps();
    
    pt.TryGetChar("key", &_p.key);
    pt.TryGetDouble("quantize", &_p.quantize);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p.actions);
    SeqAction::LoadActionsFromChildNode(pt, "stop_actions", _p.stopActions);

    switch (_p.type) {
    case MechType::Pusher:
        pt.TryGetFloat("pusher_angle", &_p.pusher.angleDeg);
        break;
    case MechType::Count: break;
    }
}

void MechEntity::InitDerived(GameManager& g) {    
    _s = State();
    InitTypeSpecificState();
    sprintf(_s.keyBuf, "%c", _p.key);
    for (auto const& pAction : _p.actions) {
        pAction->Init(g);
    }
    for (auto const& pAction : _p.stopActions) {
        pAction->Init(g);
    }

    switch (_p.type) {
    case MechType::Pusher:
        break;
    case MechType::Count: break;
    }

}

ne::Entity::ImGuiResult MechEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (MechTypeImGui("Type##mechType", &_p.type)) {
        result = ImGuiResult::NeedsInit;
        InitTypeSpecificProps();
        InitTypeSpecificState();
    }
    
    if (ImGui::InputText("Key##MechKey", _s.keyBuf, 2, ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (_s.keyBuf[0] != '\0') {
            _p.key = _s.keyBuf[0];
        } else {
            sprintf(_s.keyBuf, "%c", _p.key);
        }
    }

    ImGui::InputDouble("Quantize", &_p.quantize);

    if (SeqAction::ImGui("Actions", _p.actions)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("Stop Actions", _p.stopActions)) {
        result = ImGuiResult::NeedsInit;
    }

    switch (_p.type) {
        case MechType::Pusher:
            ImGui::InputFloat("Angle", &_p.pusher.angleDeg);
            break;
        case MechType::Count: break;
    }
    
    return result;
}

void MechEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    bool keyJustPressed = false;
    bool keyJustReleased = false;
    bool keyDown = false;
    bool quantizeReached = false;
    double quantizedBeatTime = -1.0;
    InputManager::Key k = InputManager::CharToKey(_p.key);
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
    
    switch (_p.type) {
        case MechType::Pusher: {
            if (!quantizeReached) {
                break;
            }
            PusherProps const& pusher = _p.pusher;
            // Default behavior: push in local +x direction
            for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
                ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
                assert(r);
                // TODO: could be more efficient if we precompute the quaternion
                // inverse in this function
                bool inRange = geometry::IsPointInBoundingBox2d(r->_transform.Pos(), _transform);
                if (!inRange) {
                    continue;
                }
                float constexpr kPushSpeed = 4.f;
                float angleRad = pusher.angleDeg * kDeg2Rad;
                Vec3 pushDir(cos(angleRad), 0.f, -sin(angleRad));
                r->_s.v = pushDir * kPushSpeed;
            }
            break;
        }
       case MechType::Count: break;
    }
}

void MechEntity::Draw(GameManager& g, float dt) { 
    switch (_p.type) {
        case MechType::Pusher:
            {
                Transform t = _transform;
                Vec3 p = t.Pos();
                Vec3 const& scale = t.Scale();
                p._y += 0.5f * scale._y;
                t.SetPos(p);
                g._scene->DrawCube(t.Mat4Scale(), _modelColor);
            }
            break;
        case MechType::Count: break;
    }
 
    {
        float constexpr kTextSize = 1.5f;
        Vec4 color(1.f, 1.f, 1.f, 1.f);
        char textBuf[] = "*";
        textBuf[0] = _p.key;
        g._scene->DrawTextWorld(std::string_view(textBuf, 1), _transform.Pos(), kTextSize, color);
    }
}

void MechEntity::OnEditPick(GameManager& g) {
}
