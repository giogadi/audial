#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string_view>

#include "matrix.h"
#include "serial.h"

struct GameManager;
class BoundMeshPNU;

namespace ne {

#define M_ENTITY_TYPES \
    X(Base) \
    X(Light) \
    X(Camera) \
    X(LaneNotePlayer) \
    X(LaneNoteEnemy) \
    X(Sequencer) \
    X(TypingPlayer) \
    X(FlowPlayer) \
    X(TypingEnemy) \
    X(StepSequencer) \
    X(ActionSequencer) \
    X(ParamAutomator) \
    X(FlowWall)

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
    bool operator<(EntityId const& rhs) const {
        return _id < rhs._id;
    }
};

struct BaseEntity {
    // serialized
    Mat4 _transform;
    std::string _name;
    bool _pickable = true;
    std::string _modelName;
    Vec4 _modelColor = Vec4(0.8f, 0.8f, 0.8f, 1.f);

    EntityId _id;
    BoundMeshPNU const* _model = nullptr;
    
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);

    enum class ImGuiResult { Done, NeedsInit };
    ImGuiResult ImGui(GameManager& g);

    virtual ~BaseEntity() {}

    // Init() is intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void Init(GameManager& g);
    virtual void Update(GameManager& g, float dt);
    virtual void Destroy(GameManager& g) {}
    virtual void OnEditPick(GameManager& g) {}
    virtual void DebugPrint();

protected:
    // Used by derived classes to save/load child-specific data.
    virtual void SaveDerived(serial::Ptree pt) const {};
    virtual void LoadDerived(serial::Ptree pt) {};
    virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; }
};

typedef BaseEntity Entity;

struct EntityManager {
    EntityManager();
    ~EntityManager();

    void Init();
    Entity* AddEntity(EntityType entityType);    
    Entity* GetEntity(EntityId id);
    Entity* FindEntityByName(std::string_view name);
    bool TagForDestroy(EntityId id);

    // Calls Destroy() on removed entities.
    void DestroyTaggedEntities(GameManager& g);

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

    // Iterates over all entities of all types.
    struct AllIterator {
        bool Finished() const;
        void Next();
        Entity* GetEntity();
    private:
        EntityManager* _mgr;
        int _typeIx = 0;        
        Iterator _typeIter;
        friend EntityManager;
        AllIterator() {}
    };
    AllIterator GetAllIterator();    

private:
    struct MapEntry {
        int _typeIx;
        int _entityIx;
    };
    std::unordered_map<int, MapEntry> _entityIdMap;
    int _nextId = 0;
    std::vector<EntityId> _toDestroy;

    struct Internal;
    std::unique_ptr<Internal> _p;

    // THIS IS UNSAFE! CAN'T ITERATE OVER THIS LIKE YOU THINK
    std::pair<Entity*, int> GetEntitiesOfType(EntityType entityType);

    std::pair<Entity*, int> GetEntityWithIndex(EntityId id);

    // Returns true if the given ID was indeed found and deleted.
    // Does not preserve ordering.
    // Does NOT call Destroy() on entity.
    bool RemoveEntity(EntityId idToRemove);
};

}  // namespace ne
