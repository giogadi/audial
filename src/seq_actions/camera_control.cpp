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
    camera->_desiredTargetToCameraOffset = _desiredTargetToCameraOffset;
    camera->_followEntityId = _targetEntityId;
    
    // numEntities = 0;
    // eIter = g._neEntityManager->GetIterator(ne::EntityType::FlowPlayer, &numEntities);
    // assert(numEntities == 1);
    // FlowPlayerEntity* player = static_cast<FlowPlayerEntity*>(eIter.GetEntity());
    // camera->_followEntityId = player->_id;
}

void CameraControlSeqAction::Load(GameManager& g, LoadInputs const& loadInputs, std::istream& input) {
    std::string token;
    input >> token;
    // type
    if (token == "offset") {
        input >> token;
        string_util::ToLower(token);
        // dir
        if (token == "top") {
            _desiredTargetToCameraOffset.Set(0, 5.f, -3.f);
        } else if (token == "bottom") {
            _desiredTargetToCameraOffset.Set(0, 5.f, 3.f);
        } else if (token == "left") {
            _desiredTargetToCameraOffset.Set(-3.f, 5.f, 0.f);
        } else if (token == "right") {
            _desiredTargetToCameraOffset.Set(3.f, 5.f, 0.f);
        } else if (token == "center") {
            _desiredTargetToCameraOffset.Set(0.f, 5.f, 0.f);
        } else {
            printf("CameraControlSeqAction::Load ERROR: Unexpected direction \"%s\"\n", token.c_str());
        }
    } else {
        printf("CameraControlSeqAction::Load ERROR: Unexpected type \"%s\"\n", token.c_str());
    }
}
