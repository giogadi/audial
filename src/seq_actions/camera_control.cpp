#include "seq_actions/camera_control.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "entities/camera.h"
#include "string_util.h"
#include "entities/flow_player.h"
#include "camera_util.h"
#include "renderer.h"

extern GameManager gGameManager;

void CameraControlSeqAction::ExecuteDerived(GameManager& g) {
    int numEntities = 0;
    ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::Camera, &numEntities);
    assert(numEntities == 1);
    CameraEntity* camera = static_cast<CameraEntity*>(eIter.GetEntity());
    if (_props._setTarget) {
        camera->_followEntityId = _targetEntityId;
        camera->_trackingFactor = _props._targetTrackingFactor;
    }
    if (_props._setOffset) {
        camera->_desiredTargetToCameraOffset = _props._desiredTargetToCameraOffset;
    }
    if (_props._hasMinX) {
        camera->_minX = _props._minX;
    } else {
        camera->_minX.reset();
    }
    if (_props._hasMinZ) {
        camera->_minZ = _props._minZ;
    } else {
        camera->_minZ.reset();
    }
    if (_props._hasMaxX) {
        camera->_maxX = _props._maxX;
    } else {
        camera->_maxX.reset();
    }
    if (_props._hasMaxZ) {
        camera->_maxZ = _props._maxZ;
    } else {
        camera->_maxZ.reset();
    }
    if (camera->_minX.has_value() || camera->_minZ.has_value() || camera->_maxX.has_value() || camera->_maxZ.has_value()) {
        camera->ResetConstraintBlend();
    }
}

void CameraControlSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {

    std::string token, key, value;
    while (!input.eof()) {
        {
            input >> token;
            if (token.empty()) {
                continue;
            }
            std::size_t delimIx = token.find_first_of(':');
            if (delimIx == std::string::npos) {
                printf("SpawnEnemySeqAction::Load: Token missing \":\" - \"%s\"\n", token.c_str());
                continue;
            }

            key = token.substr(0, delimIx);
            value = token.substr(delimIx+1);
        }

        if (key == "offset") {
            Direction offsetFromCamera = StringToDirection(value.c_str());
            _props._desiredTargetToCameraOffset = camera_util::GetDefaultCameraOffset(offsetFromCamera);            
            _props._setOffset = true;
        } else if (key == "target") {
            _props._targetEditorId.SetFromString(value);
            _props._setTarget = true;
        } else if (key == "tracking") {
            if (!string_util::MaybeStof(value, _props._targetTrackingFactor)) {
                printf("CameraControlSeqAction: could not parse \"%s\" as float\n", value.c_str());
            }            
        } else if (key == "min_x") {
            _props._hasMinX = string_util::MaybeStof(value, _props._minX);
        } else if (key == "max_x") {
            _props._hasMaxX = string_util::MaybeStof(value, _props._maxX);
        } else if (key == "min_z") {
            _props._hasMinZ = string_util::MaybeStof(value, _props._minZ);
        } else if (key == "max_z") {
            _props._hasMaxZ = string_util::MaybeStof(value, _props._maxZ);
        } else {
            printf("CameraControlSeqAction::Load: Unrecognized key \"%s\"\n", key.c_str());
        }
    }  
}

void CameraControlSeqAction::InitDerived(GameManager& g) {
    if (_props._targetEditorId.IsValid()) {
        ne::Entity* e = g._neEntityManager->FindEntityByEditorId(_props._targetEditorId, nullptr, "CameraControlSeqAction");
        if (e) {
            _targetEntityId = e->_id;
        }
    }
}

bool CameraControlSeqAction::ImGui() {
    if (ImGui::Button("Move Dbg cam to target")) {
        ne::Entity* e = gGameManager._neEntityManager->FindEntityByEditorId(_props._targetEditorId);
        if (e) {
            Vec3 targetPos = e->_transform.Pos();
            Vec3 newCameraPos = targetPos + _props._desiredTargetToCameraOffset;
            gGameManager._scene->_camera._transform.SetTranslation(newCameraPos);
        }
    }
    
    return _props.ImGui();
}
