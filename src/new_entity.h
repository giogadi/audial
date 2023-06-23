#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string_view>

#include "serial.h"
#include "transform.h"

struct GameManager;
class BoundMeshPNU;

namespace ne {

#define M_ENTITY_TYPES \
    X(Base) \
    X(Light) \
    X(Camera) \
    X(Sequencer) \
    X(ActionSequencer) \
    X(TypingPlayer) \
    X(TypingEnemy)  \
    X(FlowPlayer) \
    X(StepSequencer) \
    X(ParamAutomator) \
    X(FlowWall) \
    X(FlowPickup) \
    X(FlowTrigger) \
    X(IntVariable) \
    X(Vfx)

enum class EntityType: int {
#   define X(a) a,
    M_ENTITY_TYPES
#   undef X
    Count
};

static inline constexpr int gkNumEntityTypes = (int)EntityType::Count;
extern char const* const gkEntityTypeNames[];
EntityType StringToEntityType(char const* s);
extern std::size_t const gkEntitySizes[];

struct EntityId {
    bool IsValid() { return _id >= 0; }
    int _id = -1;
    EntityType _type;
    bool operator==(EntityId const& rhs) const {
        bool v = _id == rhs._id;
        if (v) {
            assert(_type == rhs._type);
        }
        return v;
    }
    bool operator!=(EntityId const& rhs) const {
        return !(*this == rhs);
    }
    bool operator<(EntityId const& rhs) const {
        return _id < rhs._id;
    }
};

struct BaseEntity {
    // serialized
    Transform _initTransform;
    bool _initActive = true;
    std::string _name;
    bool _pickable = true;
    std::string _modelName;
    Vec4 _modelColor = Vec4(0.8f, 0.8f, 0.8f, 1.f);
    int _flowSectionId = -1;

    Transform _transform;
    EntityId _id;
    BoundMeshPNU const* _model = nullptr;
    
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

protected:
    // Used by derived classes to work with child-specific data.
    virtual void InitDerived(GameManager& g) {}
    virtual void SaveDerived(serial::Ptree pt) const {};
    virtual void LoadDerived(serial::Ptree pt) {};
    virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; }
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
    Entity* GetActiveOrInactiveEntity(EntityId id, bool* active = nullptr) {
        return GetEntity(id, true, true, active);
    }
    Entity* GetEntity(EntityId id, bool includeActive, bool includeInactive, bool* active = nullptr);
    Entity* FindEntityByName(std::string_view name);
    Entity* FindInactiveEntityByName(std::string_view name);
    Entity* FindEntityByName(std::string_view name, bool includeActive, bool includeInactive);
    Entity* FindEntityByNameAndType(std::string_view name, EntityType entityType);
    Entity* GetFirstEntityOfType(EntityType entityType);
    
    bool TagForDestroy(EntityId id);
    void TagAllPrevSectionEntitiesForDestroy(int newFlowSectionId);
    void TagAllSectionEntitiesForDestroy(int sectionToDestroy);
    // Calls Destroy() on removed entities.
    void DestroyTaggedEntities(GameManager& g);

    // Only looks for active entities.
    bool TagForDeactivate(EntityId id);
    void DeactivateTaggedEntities(GameManager& g);

    // Only looks for inactive entities.
    bool TagForActivate(EntityId id, bool initOnActivate);
    void ActivateTaggedEntities(GameManager& g);

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

}  // namespace ne
