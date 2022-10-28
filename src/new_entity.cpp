#include "new_entity.h"

#include <cassert>

#include "imgui/imgui.h"
#include "imgui_util.h"
#include "game_manager.h"
#include "renderer.h"

namespace ne {

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

    std::unordered_map<std::string, EntityType> const gkStringToEntityType = {
        {"Base", EntityType::Base},
        {"Light", EntityType::Light}
    };
}

char const* const gkEntityTypeNames[] = {
    "Base", "LightEntity"
};

EntityType StringToEntityType(char const* s) {
    return gkStringToEntityType.at(s);
}

void EntityManager::Init() {
    _baseEntities.reserve(100);
    _lightEntities.reserve(100);
}

Entity* EntityManager::AddEntity(EntityType entityType) {
    Entity* e = nullptr;
    int entityIx = -1;
    switch (entityType) {
        case EntityType::Base: {
            _baseEntities.emplace_back();
            e = &(_baseEntities.back());
            entityIx = _baseEntities.size() - 1;
            break;
        }
        case EntityType::Light: {
            _lightEntities.emplace_back();
            e = &(_lightEntities.back());
            entityIx = _lightEntities.size() - 1;
            break;
        }
        case EntityType::Count: {
            assert(false);
            return nullptr;
        }
    }
    e->_id._id = _nextId++;
    e->_id._type = entityType;
    MapEntry entry;
    entry._typeIx = (int)entityType;
    entry._entityIx = entityIx;
    bool newEntry = _entityIdMap.emplace(e->_id._id, entry).second;
    assert(newEntry);
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

std::pair<Entity*, int> EntityManager::GetEntityWithIndex(EntityId id) {
    if (!id.IsValid()) {
        return std::make_pair(nullptr, -1);
    }
    auto result = _entityIdMap.find(id._id);
    if (result == _entityIdMap.end()) {
        return std::make_pair(nullptr, -1);
    }
    MapEntry const& entry = result->second;
    assert(entry._typeIx == (int)id._type);
    auto [pEntityList, numEntities] = GetEntitiesOfType(id._type);
    assert(entry._entityIx < numEntities);
    Entity* e = (Entity*) (((char*)pEntityList) + (entry._entityIx * GetEntitySize(id._type)));    
    if (e->_id._id == id._id) {
        assert(e->_id._type == id._type);
        return std::make_pair(e, entry._entityIx);
    }
    return std::make_pair(nullptr, -1);
}

Entity* EntityManager::GetEntity(EntityId id) {
    return GetEntityWithIndex(id).first;
}

bool EntityManager::RemoveEntity(EntityId idToRemove) {
    auto [toRemove, toRemoveIx] = GetEntityWithIndex(idToRemove);
    if (toRemove == nullptr) {
        return false;
    }

    // Do a swap remove from entity list, remove from id map, and update swapped entry in id map.
    // TODO: Can we get away with:
    //     *e = std::move(entities.back());
    Entity* entryToUpdate = nullptr;
    switch (idToRemove._type) {
        case EntityType::Base: {
            Entity* e = (Entity*)toRemove;  
            if (_baseEntities.size() > 1) {
                std::swap(*e, _baseEntities.back());
                entryToUpdate = e;
            }
            _baseEntities.pop_back();
            break;
        }
        case EntityType::Light: {
            LightEntity* e = (LightEntity*)toRemove;
            if (_lightEntities.size() > 1) {
                std::swap(*e, _lightEntities.back());
                entryToUpdate = e;
            }
            _lightEntities.pop_back();
            break;
        }
        case EntityType::Count: {
            assert(false);
            break;
        }
    }

    if (entryToUpdate != nullptr) {
        MapEntry& entry = _entityIdMap.at(entryToUpdate->_id._id);
        entry._entityIx = toRemoveIx;
    }

    assert(_entityIdMap.erase(idToRemove._id) == 1);

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
void Entity::Save(serial::Ptree pt) const {
    pt.PutString("name", _name.c_str());
    serial::SaveInNewChildOf(pt, "transform_mat4", _transform);
    SaveDerived(pt);
}
void Entity::Load(serial::Ptree pt) {
    _name = pt.GetString("name");
    _transform.Load(pt.GetChild("transform_mat4"));
    LoadDerived(pt);
}
void Entity::ImGui() {
    imgui_util::InputText<128>("Entity name##TopLevel", &_name);
    ImGui::Text("Entity type: %s", ne::gkEntityTypeNames[(int)_id._type]);
    ImGuiDerived();
}

void LightEntity::DebugPrint() {
    Entity::DebugPrint();
    printf("ambient: %f %f %f\n", _ambient._x, _ambient._y, _ambient._z);
    printf("diffuse: %f %f %f\n", _diffuse._x, _diffuse._y, _diffuse._z);
}
void LightEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "ambient", _ambient);
    serial::SaveInNewChildOf(pt, "diffuse", _diffuse);
}
void LightEntity::LoadDerived(serial::Ptree pt) {
    _ambient.Load(pt.GetChild("ambient"));
    _diffuse.Load(pt.GetChild("diffuse"));
}
void LightEntity::ImGuiDerived() {
    ImGui::InputScalar("Diffuse R", ImGuiDataType_Float, &_diffuse._x);
    ImGui::InputScalar("Diffuse G", ImGuiDataType_Float, &_diffuse._y);
    ImGui::InputScalar("Diffuse B", ImGuiDataType_Float, &_diffuse._z);

    ImGui::InputScalar("Ambient R", ImGuiDataType_Float, &_ambient._x);
    ImGui::InputScalar("Ambient G", ImGuiDataType_Float, &_ambient._y);
    ImGui::InputScalar("Ambient B", ImGuiDataType_Float, &_ambient._z);
}
void LightEntity::Init(GameManager& g) {
    _lightId = g._scene->AddPointLight().first;
}
void LightEntity::Update(GameManager& g, float dt) {
    renderer::PointLight* light = g._scene->GetPointLight(_lightId);
    light->Set(_transform.GetPos(), _ambient, _diffuse);
}
void LightEntity::Destroy(GameManager& g) {
    g._scene->RemovePointLight(_lightId);
}

}  // namespace ne