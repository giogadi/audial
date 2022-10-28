#pragma once

#include <vector>
#include <unordered_map>

#include "matrix.h"
#include "serial.h"
#include "version_id.h"

struct GameManager;

namespace ne {

enum class EntityType {
    Base,
    Light,
    Count
};
static inline constexpr int gkNumEntityTypes = (int)EntityType::Count;
extern char const* const gkEntityTypeNames[];
EntityType StringToEntityType(char const* s);

struct EntityId {
    bool IsValid() { return _id >= 0; }
    int _id = -1;
    EntityType _type;
};

struct Entity {
    // serialized
    Mat4 _transform;
    std::string _name;

    EntityId _id;

    virtual void DebugPrint();    
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    void ImGui();
    virtual ~Entity() {}

    // Intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void Init(GameManager& g) {}

    virtual void Update(GameManager& g, float dt) {}

    virtual void Destroy(GameManager& g) {}

protected:
    // Used by derived classes to save/load child-specific data.
    virtual void SaveDerived(serial::Ptree pt) const {};
    virtual void LoadDerived(serial::Ptree pt) {};
    virtual void ImGuiDerived() {}
};

struct LightEntity : public Entity {
    // serialized
    Vec3 _ambient;
    Vec3 _diffuse;

    VersionId _lightId;
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void Destroy(GameManager& g) override;

    virtual void DebugPrint() override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void ImGuiDerived() override;
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
    
    std::vector<Entity> _baseEntities;
    std::vector<LightEntity> _lightEntities;

private:
    struct MapEntry {
        int _typeIx;
        int _entityIx;
    };
    std::unordered_map<int, MapEntry> _entityIdMap;
    int _nextId = 0;

    // THIS IS UNSAFE! CAN'T ITERATE OVER THIS LIKE YOU THINK
    std::pair<Entity*, int> GetEntitiesOfType(EntityType entityType);

    std::pair<Entity*, int> GetEntityWithIndex(EntityId id);
};

}  // namespace ne