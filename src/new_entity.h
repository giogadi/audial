#include <vector>

#include "matrix.h"

enum class EntityType {
    Base,
    Light,
    Count
};
static inline constexpr int gkNumEntityTypes = (int)EntityType::Count;

struct EntityId {
    bool IsValid() { return _id >= 0; }
    int _id = -1;
    EntityType _type;
};

struct Entity {
    EntityId _id;
    Mat4 _transform;
    virtual void DebugPrint();
    virtual ~Entity() {}
};

struct LightEntity : public Entity {
    Vec3 _ambient;
    Vec3 _diffuse;
    virtual void DebugPrint() override;
};

extern std::size_t const gkEntitySizes[];

struct EntityManager {
    void Init();
    Entity* AddEntity(EntityType entityType);    
    Entity* GetEntity(EntityId id);
    // Returns true if the given ID was indeed found and deleted.
    // Does not preserve ordering.
    bool RemoveEntity(EntityId idToRemove);

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

    int _nextId = 0;
    std::vector<Entity> _baseEntities;
    std::vector<LightEntity> _lightEntities;

private:
    // THIS IS UNSAFE! CAN'T ITERATE OVER THIS LIKE YOU THINK
    std::pair<Entity*, int> GetEntitiesOfType(EntityType entityType);
};