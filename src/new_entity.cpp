#include "new_entity.h"

#include <cassert>

namespace {
    std::size_t GetEntitySize(EntityType entityType) {
        switch (entityType) {
            case EntityType::Base: return sizeof(Entity);
            case EntityType::Light: return sizeof(LightEntity);
            case EntityType::Count: {
                assert(false);
                return 0;
            }
        }
        return 0;
    }
}

void EntityManager::Init() {
    _baseEntities.reserve(100);
    _lightEntities.reserve(100);
}

Entity* EntityManager::AddEntity(EntityType entityType) {
    Entity* e = nullptr;
    switch (entityType) {
        case EntityType::Base: {
            _baseEntities.emplace_back();
            e = &(_baseEntities.back());
            break;
        }
        case EntityType::Light: {
            _lightEntities.emplace_back();
            e = &(_lightEntities.back());
            break;
        }
        case EntityType::Count: {
            assert(false);
            return nullptr;
        }
    }
    e->_id._id = _nextId++;
    e->_id._type = entityType;   
    return e; 
}

std::pair<Entity*, int> EntityManager::GetEntitiesOfType(EntityType type) {
    switch (type) {
        case EntityType::Base: {
            return std::make_pair(_baseEntities.data(), _baseEntities.size());
        }
        case EntityType::Light: {
            return std::make_pair(_lightEntities.data(), _lightEntities.size());
        }
        case EntityType::Count: {
            assert(false);
            return std::make_pair(nullptr, 0);
        }
    }
}

Entity* EntityManager::GetEntity(EntityId id) {
    if (!id.IsValid()) { return nullptr; }
    Iterator iter = GetIterator(id._type);
    for (; !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        if (e->_id._id == id._id) {
            return e;
        }
    }
    return nullptr;
}

bool EntityManager::RemoveEntity(EntityId idToRemove) {
    Iterator iter = GetIterator(idToRemove._type);
    Entity* toRemove = nullptr;
    for (; !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        if (e->_id._id == idToRemove._id) {
            toRemove = e;
            break;
        }
    }
    if (toRemove == nullptr) {
        return false;
    }
    // GROSS
    switch (idToRemove._type) {
        case EntityType::Base: {
            Entity* e = (Entity*)toRemove;
            *e = _baseEntities.back();
            _baseEntities.pop_back();
            break;
        }
        case EntityType::Light: {
            LightEntity* e = (LightEntity*)toRemove;
            *e = _lightEntities.back();
            _lightEntities.pop_back();
            break;
        }
        case EntityType::Count: {
            assert(false);
            break;
        }
    }
    return true;
}

EntityManager::Iterator EntityManager::GetIterator(EntityType type, int* outNumEntities) {
    Iterator iter;
    auto [entityList, numEntities] = GetEntitiesOfType(type);
    iter._stride = GetEntitySize(type);
    iter._current = entityList;
    char* p = (char*) entityList;
    char* end = p + numEntities * iter._stride;
    iter._end = (Entity*)end;
    if (outNumEntities != nullptr) {
        *outNumEntities = numEntities;
    }
    return iter;
}

bool EntityManager::Iterator::Finished() const {
    return _current >= _end;
}

Entity* EntityManager::Iterator::GetEntity() {
    assert(!Finished());
    return _current;
}

void EntityManager::Iterator::Next() {
    assert(!Finished());
    char* p = (char*)_current;
    p += _stride;
    _current = (Entity*)p;
}

EntityManager::AllIterator EntityManager::GetAllIterator() {
    AllIterator iter;
    iter._mgr = this;
    for (iter._typeIx = 0; iter._typeIx < gkNumEntityTypes; ++iter._typeIx) {
        iter._typeIter = GetIterator((EntityType)iter._typeIx);
        if (!iter._typeIter.Finished()) {
            return iter;
        }
    }
    assert(iter.Finished());
    return iter;
}

bool EntityManager::AllIterator::Finished() const {
    return _typeIx >= gkNumEntityTypes;
}

void EntityManager::AllIterator::Next() {
    if (Finished()) { 
        return;
    }
    assert(!_typeIter.Finished());
    _typeIter.Next();
    if (!_typeIter.Finished()) {
        return;
    }
    ++_typeIx;
    for (; _typeIx < gkNumEntityTypes; ++_typeIx) {
        _typeIter = _mgr->GetIterator((EntityType)_typeIx);
        if (!_typeIter.Finished()) {
            return;
        }
    }
    assert(Finished());
}

Entity* EntityManager::AllIterator::GetEntity() {
    assert(!Finished());
    return _typeIter._current;
}

void Entity::DebugPrint() {
    Vec3 p = _transform.GetPos();
    printf("pos: %f %f %f\n", p._x, p._y, p._z);
}

void LightEntity::DebugPrint() {
    Entity::DebugPrint();
    printf("ambient: %f %f %f\n", _ambient._x, _ambient._y, _ambient._z);
    printf("diffuse: %f %f %f\n", _diffuse._x, _diffuse._y, _diffuse._z);
}

