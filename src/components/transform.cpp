#include "transform.h"

#include "entity_manager.h"
#include "game_manager.h"
#include "serial.h"

bool TransformComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    if (_parentEntityName != "") {
        EntityId parentId = g._entityManager->FindActiveEntityByName(_parentEntityName.c_str());
        Entity* parentEntity = g._entityManager->GetEntity(parentId);
        if (parentEntity == &e) {
            printf("ERROR: TransformComponent parented to itself!\n");
            return false;
        }
        Mat4 worldTrans = _localTransform;
        _parent = parentEntity->FindComponentOfType<TransformComponent>();
        if (_parent.expired()) {
            return false;
        }
        // Set the local transform of this component such that its worldposition
        // remains unchanged (and establishes an offset from parent).
        SetWorldMat4(worldTrans);
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

void TransformComponent::Unparent() {
    if (_parent.expired()) {
        return;
    }
    Mat4 currentWorld = GetWorldMat4();
    _localTransform = currentWorld;
    _parent.reset();
}

void TransformComponent::Parent(std::shared_ptr<TransformComponent const> const& parent) {
    Mat4 currentWorld = GetWorldMat4();
    _parent = parent;
    SetWorldMat4(currentWorld);
}

Vec3 TransformComponent::TransformLocalToWorld(Vec3 const& v) const {
    if (std::shared_ptr<TransformComponent const> parent = _parent.lock()) {
        Vec4 v4(v._x, v._y, v._z, 1.f);
        v4 = parent->GetWorldMat4() * v4;
        return Vec3(v4._x, v4._y, v4._z);
    }
    return v;
}