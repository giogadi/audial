#include "resource.h"

#include "game_manager.h"

void ResourceEntity::SaveDerived(serial::Ptree pt) const {
}

void ResourceEntity::LoadDerived(serial::Ptree pt) {
}

void ResourceEntity::InitDerived(GameManager& g) {
    _s = State();
}

ne::Entity::ImGuiResult ResourceEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}

void ResourceEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    Vec3 p = _transform.Pos();
    Vec3 dp = _s.v * dt;
    p += dp;
    _transform.SetPos(p);
}
