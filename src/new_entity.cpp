#include "new_entity.h"

#include <cassert>
#include <vector>

#include "imgui/imgui.h"
#include "imgui_util.h"
#include "game_manager.h"
#include "renderer.h"

#include "entities/light.h"
#include "entities/camera.h"
#include "entities/lane_note_enemy.h"
#include "entities/lane_note_player.h"
#include "entities/sequencer.h"
#include "entities/typing_player.h"
#include "entities/typing_enemy.h"
#include "entities/step_sequencer.h"
#include "entities/action_sequencer.h"
#include "entities/param_automator.h"
#include "entities/flow_player.h"
#include "entities/flow_wall.h"

namespace ne {

namespace {
    std::unordered_map<std::string, EntityType> const gkStringToEntityType = {
#       define X(name) {#name, EntityType::name},
        M_ENTITY_TYPES
#       undef X
    };
}

std::size_t const gkEntitySizes[] = {
#   define X(a) sizeof(a##Entity),
    M_ENTITY_TYPES
#   undef X
};

char const* const gkEntityTypeNames[] = {
#   define X(name) #name,
    M_ENTITY_TYPES
#   undef X
};

EntityType StringToEntityType(char const* s) {
    return gkStringToEntityType.at(s);
}

struct EntityManager::Internal {
    // Entity lists
#   define X(NAME) std::vector<NAME##Entity> _entities##NAME;
    M_ENTITY_TYPES
#   undef X
};

EntityManager::EntityManager() {}

EntityManager::~EntityManager() {}


void EntityManager::Init() {
    _p = std::make_unique<Internal>();
    // _p->_entitiesBase.reserve(100);
    // _p->_entitiesLight.reserve(1);
}

Entity* EntityManager::AddEntity(EntityType entityType) {
    Entity* e = nullptr;
    int entityIx = -1;
    switch (entityType) {
#       define X(NAME) \
        case EntityType::NAME: { \
            auto& entities = _p->_entities##NAME; \
            entities.emplace_back(); \
            e = &(entities.back()); \
            entityIx = entities.size() - 1; \
            break; \
        }
        M_ENTITY_TYPES
#       undef X
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
#       define X(NAME) \
        case EntityType::NAME: { \
            auto& entities = _p->_entities##NAME; \
            return std::make_pair(entities.data(), entities.size()); \
        }
        M_ENTITY_TYPES
#       undef X        
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
    Entity* e = (Entity*) (((char*)pEntityList) + (entry._entityIx * gkEntitySizes[(int)id._type]));
    if (e->_id._id == id._id) {
        assert(e->_id._type == id._type);
        return std::make_pair(e, entry._entityIx);
    }
    return std::make_pair(nullptr, -1);
}

Entity* EntityManager::GetEntity(EntityId id) {
    return GetEntityWithIndex(id).first;
}

Entity* EntityManager::FindEntityByName(std::string_view name) {
    for (AllIterator iter = GetAllIterator(); !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        if (e->_name == name) {
            return e;
        }
    }
    return nullptr;
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
#       define X(NAME) \
        case EntityType::NAME: { \
            NAME##Entity* e = (NAME##Entity*)toRemove; \
            auto& entities = _p->_entities##NAME; \
            if (entities.size() > 1) { \
                std::swap(*e, entities.back()); \
                entryToUpdate = e; \
            } \
            entities.pop_back(); \
            break; \
        }
        M_ENTITY_TYPES
#       undef X        
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

bool EntityManager::TagForDestroy(EntityId idToDestroy) {
    if (Entity* e = GetEntity(idToDestroy)) {
        _toDestroy.push_back(idToDestroy);
        return true;
    }
    return false;
}

void EntityManager::DestroyTaggedEntities(GameManager& g) {
    for (EntityId& id : _toDestroy) {
        if (Entity* e = GetEntity(id)) {
            e->Destroy(g);
        }
        RemoveEntity(id);
    }
    _toDestroy.clear();
}

EntityManager::Iterator EntityManager::GetIterator(EntityType type, int* outNumEntities) {
    Iterator iter;
    auto [entityList, numEntities] = GetEntitiesOfType(type);
    iter._stride = gkEntitySizes[(int)type];
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

void Entity::Init(GameManager& g) {
    _model = g._scene->GetMesh(_modelName);
}
void Entity::Update(GameManager& g, float dt) {
    if (_model != nullptr) {
        g._scene->DrawMesh(_model, _transform, _modelColor);
    }
}
void Entity::DebugPrint() {
    Vec3 p = _transform.GetPos();
    printf("pos: %f %f %f\n", p._x, p._y, p._z);
}
void Entity::Save(serial::Ptree pt) const {
    pt.PutString("name", _name.c_str());
    pt.PutBool("pickable", _pickable);
    serial::SaveInNewChildOf(pt, "transform_mat4", _transform);
    pt.PutString("model_name", _modelName.c_str());
    serial::SaveInNewChildOf(pt, "model_color_vec4", _modelColor);
    SaveDerived(pt);
}
void Entity::Load(serial::Ptree pt) {
    _name = pt.GetString("name");
    pt.TryGetBool("pickable", &_pickable);
    _transform.Load(pt.GetChild("transform_mat4"));
    pt.TryGetString("model_name", &_modelName);
    serial::Ptree colorPt = pt.TryGetChild("model_color_vec4");
    if (colorPt.IsValid()) {
        _modelColor.Load(colorPt);
    }
    LoadDerived(pt);
}
Entity::ImGuiResult Entity::ImGui(GameManager& g) {
    imgui_util::InputText<128>("Entity name##TopLevel", &_name);
    ImGui::Text("Entity type: %s", ne::gkEntityTypeNames[(int)_id._type]);
    ImGui::Checkbox("Pickable", &_pickable);
    Vec3 p = _transform.GetPos();
    if (ImGui::InputFloat3("Pos##Entity", p._data, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
        _transform.SetTranslation(p);
    }
    {
        Mat3 rs = _transform.GetMat3();
        Vec3 rs0 = rs.GetRow(0);
        Vec3 rs1 = rs.GetRow(1);
        Vec3 rs2 = rs.GetRow(2);
        bool transChanged = false;
        if (ImGui::InputFloat3("RotScaleRow0##Entity", rs0._data, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            transChanged = true;
        }
        if (ImGui::InputFloat3("RotScaleRow1##Entity", rs1._data, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            transChanged = true;
        }
        if (ImGui::InputFloat3("RotScaleRow2##Entity", rs2._data, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            transChanged = true;
        }
        if (transChanged) {
            rs.SetRow(0, rs0);
            rs.SetRow(1, rs1);
            rs.SetRow(2, rs2);
            _transform.SetTopLeftMat3(rs);
        }
    }
    bool modelChanged = imgui_util::InputText<64>("Model name##Entity", &_modelName, /*trueOnReturnOnly=*/true);
    ImGui::ColorEdit4("Model color##Entity", _modelColor._data);    
    bool derivedNeedsInit = ImGuiDerived(g) == ImGuiResult::NeedsInit;
    if (modelChanged || derivedNeedsInit) {
        return ImGuiResult::NeedsInit;
    } else {
        return ImGuiResult::Done;
    }
}

}  // namespace ne
