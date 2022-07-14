#include "transform.h"

#include "entity_manager.h"
#include "game_manager.h"
#include "serial.h"

bool TransformComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    if (_parentEntityName != "") {
        EntityId parentId = g._entityManager->FindActiveEntityByName(_parentEntityName.c_str());
        Entity* parentEntity = g._entityManager->GetEntity(id);
        _parent = parentEntity->FindComponentOfType<TransformComponent>();
        if (_parent.expired()) {
            return false;
        }
    }
    return true;
}

void TransformComponent::Save(ptree& pt) const {
    _localTransform.Save(pt.add_child("mat4", ptree()));
    pt.put("parent_name", _parentEntityName);
}
void TransformComponent::Load(ptree const& pt) {
    _localTransform.Load(pt.get_child("mat4"));
    boost::optional<std::string> maybe_parent = pt.get_optional<std::string>("parent_name");
    if (maybe_parent) {
        _parentEntityName = *maybe_parent;
    }
}