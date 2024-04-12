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
    case MechType::Spawner:
        _p.spawner = SpawnerProps();
        break;
    case MechType::Pusher:
        _p.pusher = PusherProps();
        _p.pusher.angleDeg = 0.f;
        break;
    case MechType::Sink:
        _p.sink = SinkProps();
        break;
    case MechType::Grabber:
        _p.grabber = GrabberProps();
        _p.grabber.angleDeg = 0.f;
        _p.grabber.length = 1.f;
        break;
    case MechType::Count: break;
    }
}

void MechEntity::InitTypeSpecificState() {
    switch (_p.type) {
    case MechType::Spawner:
        _s.spawner = SpawnerState();
        break;
    case MechType::Pusher:
        _s.pusher = PusherState();
        break;
    case MechType::Sink:
        _s.sink = SinkState();
        break;
    case MechType::Grabber:
        _s.grabber = GrabberState();
        _s.grabber.angleRad = 0.f;
        _s.grabber.moveStartTime = -1.0;
        _s.grabber.moveEndTime = -1.0;
        memset(_s.grabber.handToResOffset, 0, sizeof(_s.grabber.handToResOffset));
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
    case MechType::Spawner:        
        break;
    case MechType::Pusher:
        pt.PutFloat("pusher_angle", _p.pusher.angleDeg);
        break;
    case MechType::Sink: 
        break;
    case MechType::Grabber:
        pt.PutFloat("grabber_angle", _p.grabber.angleDeg);
        pt.PutFloat("grabber_length", _p.grabber.length);
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
    case MechType::Spawner:        
        break;
    case MechType::Pusher:
        pt.TryGetFloat("pusher_angle", &_p.pusher.angleDeg);
        break;
    case MechType::Sink:        
        break;
    case MechType::Grabber:
        pt.TryGetFloat("grabber_angle", &_p.grabber.angleDeg);
        pt.TryGetFloat("grabber_length", &_p.grabber.length);
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
    case MechType::Spawner:        
        break;
    case MechType::Pusher:
        break;
    case MechType::Sink:        
        break;
    case MechType::Grabber:
        _s.grabber.angleRad = _p.grabber.angleDeg * kDeg2Rad;
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
    case MechType::Spawner:            
            break;
        case MechType::Pusher:
            ImGui::InputFloat("Angle", &_p.pusher.angleDeg);
            break;
        case MechType::Sink:
            break;
        case MechType::Grabber:
            if (ImGui::SliderFloat("Angle", &_p.grabber.angleDeg, -180.f, 180.f)) {
                result = ImGuiResult::NeedsInit;
            }
            ImGui::SliderFloat("Length", &_p.grabber.length, 0.f, 10.f);
            break;
        case MechType::Count: break;
    }
    
    return result;
}

namespace {
float Normalize2Pi(float angleRad) {
    return angleRad - (2*kPi) * std::floor(angleRad / (2*kPi));
}

Transform GetGrabberHandTrans(Transform grabberTrans, float grabberLength, float grabberAngleRad) {
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
        case MechType::Spawner: {
            if (quantizeReached) {
                // Check if there are any resources already in the spawner
                bool resourcePresent = false;
                for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
                    ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
                    assert(r);
                    bool inRange = geometry::IsPointInBoundingBox2d(r->_transform.Pos(), _transform);
                    if (inRange) {
                        resourcePresent = true;
                        break;
                    }
                }
                if (resourcePresent) {
                    break;
                }
                ne::Entity* r = g._neEntityManager->AddEntity(ne::EntityType::Resource);
                r->_initTransform = _transform;
                r->_initTransform.Translate(Vec3(0.f, 1.f, 0.f));
                r->_initTransform.SetScale(Vec3(0.25f, 0.25f, 0.25f));
                r->_modelName = "cube";
                r->_modelColor.Set(1.f, 1.f, 1.f, 1.f);

                r->Init(g);
            }
            break;
        }
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
        case MechType::Sink: {
            if (!keyJustPressed) {
                break;
            }
            for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
                ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
                assert(r);
                bool inRange = geometry::IsPointInBoundingBox2d(r->_transform.Pos(), _transform);
                if (!inRange) {
                    continue;
                }

                g._neEntityManager->TagForDestroy(r->_id);
            }
            break;
        }
        case MechType::Grabber: { 
            if (quantizeReached && _s.grabber.moveStartTime < 0.0) {
                _s.grabber.moveStartTime = quantizedBeatTime;
                _s.grabber.moveEndTime = quantizedBeatTime + 0.125;                
                _s.grabber.angleRad = Normalize2Pi(_s.grabber.angleRad);
                _s.grabber.moveStartAngleRad = _s.grabber.angleRad;
                _s.grabber.moveEndAngleRad = _s.grabber.moveStartAngleRad + (2 * kPi / 8);
    
                _s.resource = ne::EntityId();
            }
            if (_s.grabber.moveStartTime > 0.0) {
                Transform handTrans = GetGrabberHandTrans(_transform, _p.grabber.length, _s.grabber.angleRad);

                ResourceEntity* resInHand = g._neEntityManager->GetEntityAs<ResourceEntity>(_s.resource);
                if (resInHand == nullptr) {
                    
                    for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
                        ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
                        assert(r);
                        bool inHand = geometry::IsPointInBoundingBox2d(r->_transform.Pos(), handTrans);
                        if (inHand) {
                            _s.resource = r->_id;
                            resInHand = r;
                            Vec3 offset = r->_transform.Pos() - handTrans.Pos();
                            offset.CopyToArray(_s.grabber.handToResOffset);
                            break;
                        }
                    }
                }


                float factor = (float) math_util::InverseLerp(_s.grabber.moveStartTime, _s.grabber.moveEndTime, beatTime);   
                factor = math_util::SmoothStep(factor);
                _s.grabber.angleRad = math_util::Lerp(_s.grabber.moveStartAngleRad, _s.grabber.moveEndAngleRad, factor);
                if (beatTime >= _s.grabber.moveEndTime) {
                    _s.grabber.moveStartTime = -1.0;
                    _s.grabber.moveEndTime = -1.0;
                }

                handTrans = GetGrabberHandTrans(_transform, _p.grabber.length, _s.grabber.angleRad); 

                if (resInHand) {
                    Vec3 offset(_s.grabber.handToResOffset);
                    Vec3 p = handTrans.Pos() + offset;
                    resInHand->_transform.SetPos(p);
                }

            }
            break;
        }
        case MechType::Count: break;
    }
}

namespace {
void DrawGrabber(MechEntity& e, GameManager& g) {

    float const length = e._p.grabber.length;

    Quaternion q;
    q.SetFromAngleAxis(e._s.grabber.angleRad, Vec3(0.f, 1.f, 0.f));
    
    Transform armTrans;
    armTrans.SetQuat(q);
    armTrans.SetScale(Vec3(length, 0.25f, 0.25f));
    
    Vec3 dir = armTrans.GetXAxis();
    Vec3 p = e._transform.Pos() + (0.5f * length) * dir;
    armTrans.SetPos(p);

    g._scene->DrawCube(armTrans.Mat4Scale(), e._modelColor);
    
    p = e._transform.Pos() + length * dir;
    Transform handTrans;
    handTrans.SetQuat(q);
    handTrans.SetScale(Vec3(1.f, 0.1f, 1.f));
    handTrans.SetPos(p);
    
    g._scene->DrawCube(handTrans.Mat4Scale(), e._modelColor);
}
}

void MechEntity::Draw(GameManager& g, float dt) { 
    switch (_p.type) {
        case MechType::Grabber:
            DrawGrabber(*this, g);
            break;
        case MechType::Spawner:            
        case MechType::Pusher:
        case MechType::Sink:
            g._scene->DrawCube(_transform.Mat4Scale(), _modelColor);
            break;
        case MechType::Count: break;
    }
 
    {
        float constexpr kTextSize = 1.5f;
        Vec4 color(1.f, 1.f, 1.f, 1.f);
        char textBuf[] = "*";
        textBuf[0] = _p.key;
        // AHHHH ALLOCATION
        g._scene->DrawTextWorld(std::string(textBuf), _transform.Pos(), kTextSize, color);
    }
}

void MechEntity::OnEditPick(GameManager& g) {
}
