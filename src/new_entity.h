#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string_view>

#include "new_entity_id.h"
#include "editor_id.h"
#include "serial.h"
#include "transform.h"
#include "waypoint_follower.h"

struct GameManager;
class BoundMeshPNU;

namespace ne {

static inline constexpr int gkNumEntityTypes = (int)EntityType::Count;
extern char const* const gkEntityTypeNames[];
EntityType StringToEntityType(char const* s);
extern std::size_t const gkEntitySizes[];

struct BaseEntity {
    virtual EntityType Type() { return EntityType::Base; }
    static EntityType StaticType() { return EntityType::Base; }
    
    // serialized
    EditorId _editorId;
    Transform _initTransform;
    bool _initActive = true;
    std::string _name;
    bool _pickable = true;
    std::string _modelName;
    std::string _textureName;
    Vec4 _modelColor = Vec4(0.8f, 0.8f, 0.8f, 1.f);
    int _flowSectionId = -1;
    int _tag = 0;
    WaypointFollower::Props _wpProps;

    Transform _transform;
    EntityId _id;
    BoundMeshPNU const* _model = nullptr;
    unsigned int _textureId = 0;
    WaypointFollower _wpFollower;
    
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);

    enum class ImGuiResult { Done, NeedsInit };
    ImGuiResult ImGui(GameManager& g);

    virtual ~BaseEntity() {}

    void Init(GameManager& g);

    void SetAllTransforms(Transform const& t) { _initTransform = _transform = t; }

    // Init() is intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void Update(GameManager& g, float dt);
    virtual void Destroy(GameManager& g) {}
    virtual void OnEditPick(GameManager& g) {}
    virtual void DebugPrint();

    virtual void UpdateEditMode(GameManager& g, float dt, bool isActive);

protected:
    // Used by derived classes to work with child-specific data.
    virtual void InitDerived(GameManager& g) {}
    virtual void SaveDerived(serial::Ptree pt) const {};
    virtual void LoadDerived(serial::Ptree pt) {};
    virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; }
    virtual void UpdateDerived(GameManager& g, float dt) {}

    virtual void Draw(GameManager& g, float dt);
};

typedef BaseEntity Entity;

struct EntityManager {
    EntityManager();
    ~EntityManager();

    void Init();
    Entity* AddEntity(EntityType entityType) {
        return AddEntity(entityType, /*active=*/true);
    }
    Entity* AddEntity(EntityType entityType, bool active);
    Entity* GetEntity(EntityId id);  // only looks for active entities
    
    template<typename T>
    T* GetEntityAs(EntityId id);
    
    Entity* GetActiveOrInactiveEntity(EntityId id, bool* active = nullptr) {
        return GetEntity(id, true, true, active);
    }
    Entity* GetEntity(EntityId id, bool includeActive, bool includeInactive, bool* active = nullptr);
    Entity* FindEntityByName(std::string_view name);
    Entity* FindInactiveEntityByName(std::string_view name);
    Entity* FindEntityByName(std::string_view name, bool includeActive, bool includeInactive);
    Entity* FindEntityByNameAndType(std::string_view name, EntityType entityType);
    Entity* FindEntityByNameAndType(std::string_view name, EntityType entityType, bool includeActive, bool includeInactive);
    Entity* FindEntityByEditorId(EditorId editorId, bool* isActive = nullptr, char const* errorPrefix = nullptr);
    Entity* FindEntityByEditorIdAndType(EditorId editorId, EntityType entityType, bool* isActive = nullptr, char const* errorPrefix = nullptr);
    Entity* GetFirstEntityOfType(EntityType entityType);

    void FindEntitiesByTag(int tag, bool includeActive, bool includeInactive, std::vector<ne::Entity*>* entities);
    void FindEntitiesByTagAndType(int tag, EntityType entityType, bool includeActive, bool includeInactive, std::vector<ne::Entity*>* entities);
    
    bool TagForDestroy(EntityId id);
    void TagAllPrevSectionEntitiesForDestroy(int newFlowSectionId);
    void TagAllSectionEntitiesForDestroy(int sectionToDestroy);
    // Calls Destroy() on removed entities.
    void DestroyTaggedEntities(GameManager& g);

    // Only looks for active entities.
    bool TagForDeactivate(EntityId id);
    void DeactivateTaggedEntities(GameManager& g);

    bool TagForActivate(EntityId id, bool initOnActivate, bool initIfAlreadyActive=false);
    void ActivateTaggedEntities(GameManager& g);

    void GetEntitiesOfType(ne::EntityType entityType, bool includeActive, bool includeInactive, std::vector<Entity*>& entitiesOut);

    // Iterates over all entities of a given type.
    struct Iterator {
        Entity* GetEntity();
        bool Finished() const;
        void Next();
    private:
        Entity* _current = nullptr;
        Entity* _end = nullptr;
        std::size_t _stride = 0;
        Iterator() {}
        friend EntityManager;
    };
    Iterator GetIterator(EntityType type, int* outNumEntities = nullptr);
    Iterator GetInactiveIterator(EntityType type, int* outNumEntities = nullptr);

    // Iterates over all entities of all types.
    struct AllIterator {
        bool Finished() const;
        void Next();
        Entity* GetEntity();
    private:
        EntityManager* _mgr;
        bool _active = true; // whether I iterate over active or inactive entities
        int _typeIx = 0;
        Iterator _typeIter;
        friend EntityManager;
        AllIterator() {}
    };
    AllIterator GetAllIterator();
    AllIterator GetAllInactiveIterator();

private:
    struct MapEntry {
        int _typeIx;
        int _entityIx;
        bool _active = true;
    };
    std::unordered_map<int, MapEntry> _entityIdMap;
    int _nextId = 0;
    std::vector<EntityId> _toDestroy;
    std::vector<EntityId> _toDeactivate;
    // {entityId, initOnActivate}
    std::vector<std::pair<EntityId, bool>> _toActivate;

    struct Internal;
    std::unique_ptr<Internal> _p;

    // THIS IS UNSAFE! CAN'T ITERATE OVER THIS LIKE YOU THINK
    std::pair<Entity*, int> GetEntitiesOfType(EntityType entityType, bool active);

    struct EntityInfo {
        Entity* _e = nullptr;
        int _ix = -1;
        bool _active = true;
    };
    EntityInfo GetEntityInfo(EntityId id, bool includeActive, bool includeInactive);

    // Returns true if the given ID was indeed found and deleted.
    // Does not preserve ordering.
    // Does NOT call Destroy() on entity.
    bool RemoveEntity(EntityId idToRemove);

    bool DeactivateEntity(EntityId idToDeactivate);

    bool ActivateEntity(EntityId idToActivate);
};

template<typename T>
T* EntityManager::GetEntityAs(EntityId id) {
    Entity* e = GetEntityInfo(id, true, false)._e;
    if (e == nullptr) {
        return nullptr;
    }
    EntityType dynType = e->Type();
    EntityType staticType = T::StaticType();
    if (staticType != EntityType::Base && dynType != staticType) {
        char const* const dynTypeName = gkEntityTypeNames[(int)dynType];
        char const* const staticTypeName = gkEntityTypeNames[(int)staticType];
        printf("EntityManager::GetEntityAs: ERROR: entity \"%s\" is of type \"%s\" but we tried to get as \"%s\".\n", e->_name.c_str(), dynTypeName, staticTypeName);
        return nullptr;
    }
    return static_cast<T*>(e);
}

}  // namespace ne
