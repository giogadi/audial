#include "seq_actions/camera_control.h"

#include "game_manager.h"
#include "entities/camera.h"
#include "string_util.h"
#include "entities/flow_player.h"
#include "camera_util.h"

void CameraControlSeqAction::Execute(GameManager& g) {
    int numEntities = 0;
    ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::Camera, &numEntities);
    assert(numEntities == 1);
    CameraEntity* camera = static_cast<CameraEntity*>(eIter.GetEntity());
    if (_setTarget) {
        camera->_followEntityId = _targetEntityId;
    }
    if (_setOffset) {
        camera->_desiredTargetToCameraOffset = _desiredTargetToCameraOffset;
    }    
    camera->_minX = _minX;
    camera->_minZ = _minZ;
    camera->_maxX = _maxX;
    camera->_maxZ = _maxZ;
    if (camera->_minX.has_value() || camera->_minZ.has_value() || camera->_maxX.has_value() || camera->_maxZ.has_value()) {
        camera->ResetConstraintBlend();
    }
}

void CameraControlSeqAction::Load(LoadInputs const& loadInputs, std::istream& input) {

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
            _desiredTargetToCameraOffset = camera_util::GetDefaultCameraOffset(offsetFromCamera);            
            _setOffset = true;
        } else if (key == "target") {
            _targetEntityName = value;
            _setTarget = true;
        } else if (key == "min_x") {            
            _minX = string_util::MaybeStof(value);
        } else if (key == "max_x") {
            _maxX = string_util::MaybeStof(value);
        } else if (key == "min_z") {
            _minZ = string_util::MaybeStof(value);
        } else if (key == "max_z") {
            _maxZ = string_util::MaybeStof(value);
        } else {
            printf("CameraControlSeqAction::Load: Unrecognized key \"%s\"\n", key.c_str());
        }
    }  
}

void CameraControlSeqAction::Init(GameManager& g) {
    if (!_targetEntityName.empty()) {
        ne::Entity* e = g._neEntityManager->FindEntityByName(_targetEntityName);
        if (e) {
            _targetEntityId = e->_id;
        } else {
            printf("SetStepSequenceSeqAction: could not find seq entity \"%s\"\n", _targetEntityName.c_str());
        }
    }
}
