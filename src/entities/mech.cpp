#include "mech.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "input_manager.h"
#include "renderer.h"
#include "entity_manager.h"
#include "geometry.h"
#include "entities/resource.h"

void MechEntity::SaveDerived(serial::Ptree pt) const {
    serial::PutEnum(pt, "mech_type", _p.type);
    pt.PutChar("key", _p.key);
}

void MechEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    serial::TryGetEnum(pt, "mech_type", _p.type);
    pt.TryGetChar("key", &_p.key);
}

void MechEntity::InitDerived(GameManager& g) {
    _s = State();
    sprintf(_s.keyBuf, "%c", _p.key);
}

ne::Entity::ImGuiResult MechEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (MechTypeImGui("Type##mechType", &_p.type)) {
        result = ImGuiResult::NeedsInit;
    }
    
    if (ImGui::InputText("Key##MechKey", _s.keyBuf, 2, ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (_s.keyBuf[0] != '\0') {
            _p.key = _s.keyBuf[0];
        } else {
            sprintf(_s.keyBuf, "%c", _p.key);
        }
    }
    
    return result;
}

void MechEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }
    
    InputManager::Key k = InputManager::CharToKey(_p.key);
    bool const keyPressed = g._inputManager->IsKeyPressedThisFrame(k);    
    
    switch (_p.type) {
        case MechType::Spawner: {
            if (keyPressed) {
                ne::Entity* r = g._neEntityManager->AddEntity(ne::EntityType::Resource);
                r->_initTransform = _transform;
                r->_initTransform.SetScale(Vec3(0.25f, 0.25f, 0.25f));
                r->_modelName = "cube";
                r->_modelColor.Set(1.f, 1.f, 1.f, 1.f);

                r->Init(g);
            }
            break;
        }
        case MechType::Pusher: {
            if (!keyPressed) {
                break;
            }
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
                r->_s.v = _transform.GetXAxis() * kPushSpeed;
            }
            break;
        }
        default: {
            break;
        }
    }
}

void MechEntity::Draw(GameManager& g, float dt) {
    g._scene->DrawCube(_transform.Mat4Scale(), _modelColor);
    
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
