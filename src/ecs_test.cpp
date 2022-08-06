#include <bitset>
#include <cassert>
#include <vector>
#include <string>

typedef int32_t EntityIndex;
typedef int32_t EntityVersion;

class EntityId {
public:
    constexpr EntityId(EntityIndex index, EntityVersion version) :
        _id(((int64_t)index << 32) | ((int64_t) version)) {}

    EntityIndex GetIndex() const {
        return _id >> 32;
    }
    EntityVersion GetVersion() const {
        return (int32_t)_id;
    }
    static constexpr EntityId InvalidId() {
        return EntityId(-1, 0);
    }
    bool IsValid() const {
        return GetIndex() >= 0;
    }
    bool operator==(EntityId rhs) const {
        return _id == rhs._id;
    }
    bool operator!=(EntityId rhs) const {
        return !(*this == rhs);
    }    
private:
    int64_t _id;
};

int constexpr kMaxComponents = 64;
typedef std::bitset<kMaxComponents> ComponentMask;

int constexpr kMaxEntities = 256;

class ComponentPool {
public:
    void Init(int elementSize) {
        assert(_pData == nullptr);
        _elementSize = elementSize;
        _pData = new char[kMaxEntities * elementSize];
    }
    ~ComponentPool() {
        delete[] _pData;
    }
    void* Get(int index) {
        assert(index < kMaxEntities);
        return _pData + (index * _elementSize);
    }
    bool IsInit() const { return _pData != nullptr; }

    char* _pData = nullptr;
    int _elementSize = 0;
};

class Scene {
public:
    struct EntityInfo {
        EntityId _id;
        ComponentMask _mask;
    };

    Scene() {
        _componentPools.reserve(kMaxComponents);
    }

    EntityId AddEntity() {        
        if (_freeEntities.empty()) {
            EntityId newId(_entities.size(), 0);
            _entities.push_back({newId, ComponentMask()});
            return newId;
        }
        EntityIndex index = _freeEntities.back();
        _freeEntities.pop_back();
        EntityId newId(index, _entities[index]._id.GetVersion());
        _entities[index] = {newId, ComponentMask()};
        return newId;
    }

    template <typename T>
    T* AddComponent(EntityId id) {
        int componentId = T::Id();
        if (componentId >= _componentPools.size()) {
            _componentPools.resize(componentId + 1);
            if (_componentPools.size() >=kMaxComponents) {
                assert(false);
            }
        }
        ComponentPool& pool = _componentPools[componentId];
        if (!pool.IsInit()) {
            pool.Init(sizeof(T));
        }
        T* pComp = new (pool.Get(id.GetIndex())) T();
        _entities[id.GetIndex()]._mask.set(componentId);
        return pComp;
    }

    template <typename T>
    T* GetComponent(EntityId id) {
        int componentId = T::Id();
        EntityInfo const& entity = _entities[id.GetIndex()];
        if (!entity._mask.test(componentId)) {
            return nullptr;
        }
        T* pComp = static_cast<T*>(_componentPools[componentId].Get(id.GetIndex()));
        return pComp;
    }

    // Index by and EntityId
    std::vector<EntityInfo> _entities;

    std::vector<EntityIndex> _freeEntities;

    // Indexed by ComponentId
    // LMAO THIS IS ACTUALLY TERRIBLE. We can't have this be a resizable vector.
    std::vector<ComponentPool> _componentPools;
};

extern int s_componentCounter;
template <class T>
int GetId()
{
  static int s_componentId = s_componentCounter++;
  return s_componentId;
}

struct ComponentFoo {
    static int Id() { return 0; }
    std::string _theStr;
    float _theFloat = 2.f;
};

struct ComponentBar {
    static int Id() { return 1; }
    bool _theBool = true;
    int _theInt = -3;
};

int main() {
    Scene scene;
    EntityId entityId1 = scene.AddEntity();
    {
        ComponentFoo* foo = scene.AddComponent<ComponentFoo>(entityId1);
        assert(foo);
        foo->_theStr = "howdy";

        ComponentBar* bar = scene.AddComponent<ComponentBar>(entityId1);
        assert(bar);
        bar->_theInt = 999;

        int blah = 0;
    }

    EntityId entityId2 = scene.AddEntity();
    {
        ComponentBar* bar = scene.AddComponent<ComponentBar>(entityId2);
        assert(bar);
        bar->_theInt = 4;
    }

    ComponentFoo* foo = scene.GetComponent<ComponentFoo>(entityId1);
    assert(foo->_theStr == "howdy");

    ComponentBar* bar = scene.GetComponent<ComponentBar>(entityId1);
    assert(bar->_theInt == 999);

    bar = scene.GetComponent<ComponentBar>(entityId2);
    assert(bar->_theInt == 4);

    return 0;
}