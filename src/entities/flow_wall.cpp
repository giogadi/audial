#include "flow_wall.h"

void FlowWallEntity::InitDerived(GameManager& g) {
    _wpFollower.Init(_transform.Pos());
}

void FlowWallEntity::Update(GameManager& g, float dt) {

    if (!g._editMode) {
        Vec3 newPos;
        bool updatePos = _wpFollower.Update(g, dt, &newPos);
        if (updatePos) {
            _transform.SetPos(newPos);
        }       
    }

    BaseEntity::Update(g, dt);
}

void FlowWallEntity::SaveDerived(serial::Ptree pt) const {
    _wpFollower.Save(pt);
}

void FlowWallEntity::LoadDerived(serial::Ptree pt) {
    _wpFollower.Load(pt);
}

FlowWallEntity::ImGuiResult FlowWallEntity::ImGuiDerived(GameManager& g)  {
    _wpFollower.ImGui();
    return ImGuiResult::Done;
}