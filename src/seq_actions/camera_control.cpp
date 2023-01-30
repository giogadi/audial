#include "seq_actions/camera_control.h"

#include "game_manager.h"
#include "entities/camera.h"
#include "string_util.h"
#include "entities/flow_player.h"

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
            if (string_util::EqualsIgnoreCase(value, "top")) {
                _desiredTargetToCameraOffset.Set(0.f, 5.f, 3.f);
            } else if (string_util::EqualsIgnoreCase(value, "bottom")) {
                _desiredTargetToCameraOffset.Set(0.f, 5.f, -3.f);
            } else if (string_util::EqualsIgnoreCase(value, "left")) {
                _desiredTargetToCameraOffset.Set(3.f, 5.f, 0.f);
            } else if (string_util::EqualsIgnoreCase(value, "right")) {
                _desiredTargetToCameraOffset.Set(-3.f, 5.f, 0.f);
            } else if (string_util::EqualsIgnoreCase(value, "center")) {
                _desiredTargetToCameraOffset.Set(0.f, 5.f, 0.f);
            } else {
                printf("SpawnEnemy:camera_offset: unrecognized direction \"%s\"\n", value.c_str());
            }
            _setOffset = true;
        } else if (key == "target") {
            _targetEntityName = value;
            _setTarget = true;
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
