#include "new_entity.h"

#include <cassert>
#include <vector>

#include "imgui/imgui.h"
#include "imgui_util.h"
#include "game_manager.h"
#include "renderer.h"
#include "enums/Direction.h"
#include "camera_util.h"

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
#include "entities/flow_pickup.h"
#include "entities/flow_trigger.h"
#include "entities/int_variable.h"

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

    // Inactive entity lists
#   define X(NAME) std::vector<NAME##Entity> _inactiveEntities##NAME;
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

Entity* EntityManager::AddEntity(EntityType entityType, bool const active) {
    Entity* e = nullptr;
    int entityIx = -1;
    switch (entityType) {
#       define X(NAME) \
        case EntityType::NAME: { \
            auto& entities = active ? _p->_entities##NAME : _p->_inactiveEntities##NAME; \
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
    entry._active = active;
    bool newEntry = _entityIdMap.emplace(e->_id._id, entry).second;
    assert(newEntry);
    return e; 
}

std::pair<Entity*, int> EntityManager::GetEntitiesOfType(EntityType type, bool active) {
    switch (type) {
#       define X(NAME) \
        case EntityType::NAME: { \
            if (active) { \
                auto& entities = _p->_entities##NAME; \
                return std::make_pair(entities.data(), entities.size()); \
            } else { \
                auto& entities = _p->_inactiveEntities##NAME; \
                return std::make_pair(entities.data(), entities.size()); \
            } \
        }
        M_ENTITY_TYPES
#       undef X
        case EntityType::Count: {
            assert(false);
            return std::make_pair(nullptr, 0);
        }
    }
    assert(false);
    return std::make_pair(nullptr, 0);
}

ne::EntityManager::EntityInfo EntityManager::GetEntityInfo(EntityId id, bool includeActive, bool includeInactive) {
    if (!id.IsValid()) {
        return EntityInfo();
    }
    auto result = _entityIdMap.find(id._id);
    if (result == _entityIdMap.end()) {
        return EntityInfo();
    }
    MapEntry const& entry = result->second;
    assert(entry._typeIx == (int)id._type);
    if (entry._active && !includeActive) {
        return EntityInfo();
    }
    if (!entry._active && !includeInactive) {
        return EntityInfo();
    }
    auto [pEntityList, numEntities] = GetEntitiesOfType(id._type, entry._active);
    assert(entry._entityIx < numEntities);
    Entity* e = (Entity*) (((char*)pEntityList) + (entry._entityIx * gkEntitySizes[(int)id._type]));
    if (e->_id._id == id._id) {
        assert(e->_id._type == id._type);
        return EntityInfo{e, entry._entityIx, entry._active};
    }
    return EntityInfo();
}

Entity* EntityManager::GetEntity(EntityId id) {
    return GetEntityInfo(id, true, false)._e;
}

Entity* EntityManager::GetEntity(EntityId id, bool includeActive, bool includeInactive, bool* active) {
    EntityInfo eInfo = GetEntityInfo(id, includeActive, includeInactive);
    if (active != nullptr) {
        *active = eInfo._active;
    }
    return eInfo._e;
}

Entity* EntityManager::GetFirstEntityOfType(EntityType entityType) {
    int numEntities;
    Iterator iter = GetIterator(entityType, &numEntities);
    if (numEntities == 0) {
        return nullptr;
    }
    return iter.GetEntity();
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

Entity* EntityManager::FindInactiveEntityByName(std::string_view name) {
    for (AllIterator iter = GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        if (e->_name == name) {
            return e;
        }
    }
    return nullptr;
}

Entity* EntityManager::FindEntityByNameAndType(std::string_view name, EntityType entityType) {
    for (Iterator iter = GetIterator(entityType); !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        if (e->_name == name) {
            return e;
        }
    }
    return nullptr;
}

bool EntityManager::RemoveEntity(EntityId idToRemove) {
    EntityInfo entityInfo = GetEntityInfo(idToRemove, true, true);
    Entity* toRemove = entityInfo._e;
    int toRemoveIx = entityInfo._ix;
    bool active = entityInfo._active;
    
    if (toRemove == nullptr) {
        return false;
    }

    // Do a swap remove from entity list, remove from id map, and update swapped entry in id map.
    Entity* entryToUpdate = nullptr;
    switch (idToRemove._type) {
#       define X(NAME) \
        case EntityType::NAME: { \
            NAME##Entity* e = (NAME##Entity*)toRemove; \
            auto& entities = active ? _p->_entities##NAME : _p->_inactiveEntities##NAME; \
            if (entities.size() > 1) { \
                *e = std::move(entities.back()); \
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

    int numErased = _entityIdMap.erase(idToRemove._id);
    assert(numErased == 1);

    return true;
}

bool EntityManager::DeactivateEntity(EntityId idToDeactivate) {    
    EntityInfo info = GetEntityInfo(idToDeactivate, true, false);
    if (info._e == nullptr) {
        return false;
    }

    Entity* entryToUpdate = nullptr;
    int inactiveEntitiesIx = -1;
    switch (idToDeactivate._type) {
#       define X(NAME) \
        case EntityType::NAME: { \
            NAME##Entity* entity = (NAME##Entity*)info._e; \
            auto& entities = _p->_entities##NAME; \
            auto& inactiveEntities = _p->_inactiveEntities##NAME; \
            inactiveEntities.push_back(std::move(*entity)); \
            inactiveEntitiesIx = inactiveEntities.size() - 1; \
            if (entities.size() > 1) { \
                *entity = std::move(entities.back()); \
                entryToUpdate = entity; \
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
        entry._entityIx = info._ix;   
    }

    // TODO we shouldn't need to do this; we've already done this lookup in GetEntityInfo.
    MapEntry& entry = _entityIdMap.at(idToDeactivate._id);
    entry._active = false;
    assert(inactiveEntitiesIx >= 0);
    entry._entityIx = inactiveEntitiesIx;
    return true;
}

bool EntityManager::ActivateEntity(EntityId idToActivate) {    
    EntityInfo info = GetEntityInfo(idToActivate, false, true);
    if (info._e == nullptr) {
        return false;
    }

    Entity* entryToUpdate = nullptr;
    int entitiesIx = -1;
    switch (idToActivate._type) {
#       define X(NAME) \
        case EntityType::NAME: { \
            NAME##Entity* entity = (NAME##Entity*)info._e; \
            auto& entities = _p->_entities##NAME; \
            auto& inactiveEntities = _p->_inactiveEntities##NAME; \
            entities.push_back(std::move(*entity)); \
            entitiesIx = entities.size() - 1; \
            if (inactiveEntities.size() > 1) { \
                *entity = std::move(inactiveEntities.back()); \
                entryToUpdate = entity; \
            } \
            inactiveEntities.pop_back(); \
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
        entry._entityIx = info._ix;   
    }

    // TODO we shouldn't need to do this; we've already done this lookup in GetEntityInfo.
    MapEntry& entry = _entityIdMap.at(idToActivate._id);
    entry._active = true;
    assert(entitiesIx >= 0);
    entry._entityIx = entitiesIx;
    return true;
}

bool EntityManager::TagForDestroy(EntityId idToDestroy) {
    if (Entity* e = GetEntity(idToDestroy, true, true)) {
        _toDestroy.push_back(idToDestroy);
        return true;
    }
    return false;
}

void EntityManager::TagAllPrevSectionEntitiesForDestroy(int newFlowSectionId) {
    for (AllIterator iter = GetAllIterator(); !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        if (e->_flowSectionId >= 0 && e->_flowSectionId < newFlowSectionId) {
            TagForDestroy(e->_id);
        }
    }
    for (AllIterator iter = GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        if (e->_flowSectionId >= 0 && e->_flowSectionId < newFlowSectionId) {
            TagForDestroy(e->_id);
        }
    }
}

void EntityManager::DestroyTaggedEntities(GameManager& g) {
    for (EntityId& id : _toDestroy) {
        if (Entity* e = GetEntity(id, true, true)) {
            e->Destroy(g);
        }
        RemoveEntity(id);
    }
    _toDestroy.clear();
}

bool EntityManager::TagForDeactivate(EntityId idToDeactivate) {
    if (Entity* e = GetEntity(idToDeactivate, true, false)) {
        _toDeactivate.push_back(idToDeactivate);
        return true;
    }
    return false;
}

void EntityManager::DeactivateTaggedEntities(GameManager& g) {
    for (EntityId& id : _toDeactivate) {
        if (Entity* e = GetEntity(id, true, false)) {
            e->Destroy(g);
        }
        DeactivateEntity(id);
    }
    _toDeactivate.clear();
}

bool EntityManager::TagForActivate(EntityId idToActivate, bool initOnActivate) {
    if (Entity* e = GetEntity(idToActivate, false, true)) {
        _toActivate.push_back({idToActivate, initOnActivate});
        return true;
    }
    return false;
}

void EntityManager::ActivateTaggedEntities(GameManager& g) {
    for (auto &[id, initOnActivate] : _toActivate) {
        ActivateEntity(id);
        if (initOnActivate) {
            Entity* e = GetEntity(id);
            e->Init(g);
        }
    }
    _toActivate.clear();
}

EntityManager::Iterator EntityManager::GetIterator(EntityType type, int* outNumEntities) {
    Iterator iter;
    auto [entityList, numEntities] = GetEntitiesOfType(type, /*active=*/true);
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

EntityManager::Iterator EntityManager::GetInactiveIterator(EntityType type, int* outNumEntities) {
    Iterator iter;
    auto [entityList, numEntities] = GetEntitiesOfType(type, /*active=*/false);
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
    iter._active = true;
    for (iter._typeIx = 0; iter._typeIx < gkNumEntityTypes; ++iter._typeIx) {
        iter._typeIter = GetIterator((EntityType)iter._typeIx);
        if (!iter._typeIter.Finished()) {
            return iter;
        }
    }
    assert(iter.Finished());
    return iter;
}

EntityManager::AllIterator EntityManager::GetAllInactiveIterator() {
    AllIterator iter;
    iter._mgr = this;
    iter._active = false;
    for (iter._typeIx = 0; iter._typeIx < gkNumEntityTypes; ++iter._typeIx) {
        iter._typeIter = GetInactiveIterator((EntityType)iter._typeIx);
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
        if (_active) {
            _typeIter = _mgr->GetIterator((EntityType)_typeIx);
        } else {
            _typeIter = _mgr->GetInactiveIterator((EntityType)_typeIx);
        }
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
    _transform = _initTransform;
    _model = g._scene->GetMesh(_modelName);
    InitDerived(g);
}
void Entity::Update(GameManager& g, float dt) {
    if (_model != nullptr) {
        Mat4 const& mat = _transform.Mat4Scale();
        g._scene->DrawMesh(_model, mat, _modelColor);
    }
}
void Entity::DebugPrint() {
    Vec3 p = _transform.GetPos();
    printf("pos: %f %f %f\n", p._x, p._y, p._z);
}
void Entity::Save(serial::Ptree pt) const {    
    pt.PutString("name", _name.c_str());
    pt.PutBool("entity_active", _initActive);
    pt.PutBool("pickable", _pickable);
    // NOTE: we always save the current position! maybe do this different
    serial::SaveInNewChildOf(pt, "transform", _initTransform);
    pt.PutString("model_name", _modelName.c_str());
    serial::SaveInNewChildOf(pt, "model_color_vec4", _modelColor);
    pt.PutInt("flow_section_id", _flowSectionId);
    SaveDerived(pt);
}
void Entity::Load(serial::Ptree pt) {
    _name = pt.GetString("name");
    _initActive = true;
    pt.TryGetBool("entity_active", &_initActive);
    pt.TryGetBool("pickable", &_pickable);
    {
        bool success = serial::LoadFromChildOf(pt, "transform", _initTransform);
        if (!success) {
            // Fallback to mat4 loading
            Mat4 transMat4;
            serial::LoadFromChildOf(pt, "transform_mat4", transMat4);
            _initTransform.SetFromMat4(transMat4);
        }
    }
    pt.TryGetString("model_name", &_modelName);
    serial::Ptree colorPt = pt.TryGetChild("model_color_vec4");
    if (colorPt.IsValid()) {
        _modelColor.Load(colorPt);
    }
    pt.TryGetInt("flow_section_id", &_flowSectionId);
    LoadDerived(pt);
}
Entity::ImGuiResult Entity::ImGui(GameManager& g) {
    imgui_util::InputText<128>("Entity name##TopLevel", &_name);
    ImGui::Text("Entity type: %s", ne::gkEntityTypeNames[(int)_id._type]);
    bool activeChanged = ImGui::Checkbox("Entity active", &_initActive);
    if (activeChanged) {
        if (_initActive) {
            g._neEntityManager->TagForActivate(_id, /*initOnActivate=*/true);
        } else {
            g._neEntityManager->TagForDeactivate(_id);
        }
    }
    ImGui::Checkbox("Pickable", &_pickable);
    Entity::ImGuiResult result = Entity::ImGuiResult::Done;
    if (ImGui::Button("Force Init")) {
        result = Entity::ImGuiResult::NeedsInit;
    }
    Transform& trans = _transform;

    Vec3 p = trans.Pos();
    if (imgui_util::InputVec3("Pos##Entity", &p, /*trueOnReturnOnly=*/true)) {
        trans.SetPos(p);
        _initTransform = trans;
    }

    Vec3 euler = trans.Quat().EulerAngles();
    euler *= kRad2Deg;
    if (imgui_util::InputVec3("Euler angles (deg)##Entity", &euler, true)) {
        euler *= kDeg2Rad;
        Quaternion q = trans.Quat();
        q.SetFromEulerAngles(euler);
        // DEBUG
        Vec3 eulerResult = q.EulerAngles();
        float diff = (euler - eulerResult).Length();
        if (diff > 0.001f) {
            printf("EULER PROBLEM:\ninput: %f %f %f\noutput: %f %f %f\n",
                   euler._x, euler._y, euler._z,
                   eulerResult._x, eulerResult._y, eulerResult._z);
        }
        trans.SetQuat(q);
        _initTransform = trans;
    }
    Vec3 scale = trans.Scale();
    if (imgui_util::InputVec3("Scale##Entity", &scale, true)) {
        float constexpr eps = 0.001f;
        scale._x = std::max(eps, scale._x);
        scale._y = std::max(eps, scale._y);
        scale._z = std::max(eps, scale._z);
        trans.SetScale(scale);
        _initTransform = trans;
    }
    ImGui::InputInt("Flow section ID##Entity", &_flowSectionId);
    bool modelChanged = imgui_util::InputText<64>("Model name##Entity", &_modelName, /*trueOnReturnOnly=*/true);
    imgui_util::ColorEdit4("Model color##Entity", &_modelColor);
    
    static int sCurDbgLookAtIx = 0;
    ImGui::Combo("##DbgLookAtOffsetType", &sCurDbgLookAtIx, gDirectionStrings, static_cast<int>(Direction::Count));
    ImGui::SameLine();
    if (ImGui::Button("DbgLookAt w/ offset")) {
        Direction offsetFromCamera = static_cast<Direction>(sCurDbgLookAtIx);
        Vec3 targetToCamera = camera_util::GetDefaultCameraOffset(offsetFromCamera);
        Vec3 cameraWorld = _transform.Pos() + targetToCamera;
        g._scene->_camera._transform.SetTranslation(cameraWorld);
    }
    bool derivedNeedsInit = ImGuiDerived(g) == ImGuiResult::NeedsInit;
    if (modelChanged || derivedNeedsInit) {
        result = Entity::ImGuiResult::NeedsInit;
    }
    return result;
}

}  // namespace ne
