#include "resource.h"

void ResourceEntity::SaveDerived(serial::Ptree pt) const {
}

void ResourceEntity::LoadDerived(serial::Ptree pt) {
}

void ResourceEntity::InitDerived(GameManager& g) {
}

ne::Entity::ImGuiResult ResourceEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}

void ResourceEntity::UpdateDerived(GameManager& g, float dt) {
    
}
