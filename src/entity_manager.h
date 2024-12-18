#pragma once

#include <functional>

#include "entity.h"

// TODO: move these inside EntityManager
enum class EntityDestroyType {
    None, DestroyComponents, EditDestroyComponents
};
struct EntityAndStatus {
    std::shared_ptr<Entity> _e;
    EntityId _id = EntityId::InvalidId();
    bool _active = true;
    EntityDestroyType _destroy = EntityDestroyType::None;
};

class EntityManager {
public:
    EntityId AddEntity(bool active=true);

    // ONLY FOR USE IN EDIT MODE TO MAINTAIN ORDERING OF ENTITIES
    EntityId AddEntityToBack(bool active=true) {
        _entities.emplace_back();
        EntityAndStatus* e_s = &_entities.back();
        e_s->_id = EntityId(_entities.size() - 1, 0);
        e_s->_e = std::make_shared<Entity>();
        e_s->_active = active;
        e_s->_destroy = EntityDestroyType::None;
        return e_s->_id;
    }

    void TagEntityForDestroy(
        EntityId idToDestroy, EntityDestroyType destroyType = EntityDestroyType::DestroyComponents);

    void Update(float dt);

    void EditModeUpdate(float dt);

    void ConnectComponents(GameManager& g, bool dieOnConnectFailure);

    EntityId FindActiveEntityByName(char const* name);
    EntityId FindInactiveEntityByName(char const* name);
    EntityId FindActiveOrInactiveEntityByName(char const* name);

    // USES EditDestroy() on deactivate!!!!
    void DeactivateEntity(EntityId id);

    void ActivateEntity(EntityId id, GameManager& g);

    void DestroyTaggedEntities();

    void ForEveryActiveEntity(std::function<void(EntityId)> f) const;
    void ForEveryActiveAndInactiveEntity(std::function<void(EntityId)> f) const;

    // DO NOT STORE POINTER!!!
    // NOTE: these return both active and inactive entities.
    // TODO: Should these consider Active status before returning?
    Entity* GetEntity(EntityId id);
    Entity const* GetEntity(EntityId id) const;

    bool IsActive(EntityId id) const;

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool LoadAndConnect(serial::Ptree pt, bool dieOnConnectFailure, GameManager& g);

private:
    // DON'T STORE THIS POINTER! Maybe should be edit-only or something
    EntityAndStatus* GetEntityAndStatus(EntityId id);
    EntityAndStatus const* GetEntityAndStatus(EntityId id) const;

    std::vector<EntityAndStatus> _entities;
    std::vector<EntityIndex> _freeList;
};