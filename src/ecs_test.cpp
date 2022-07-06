#include <bitset>

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
        assert(_pdata == nullptr);
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
        _entities[index] = newId;
        return newId;
    }

    template <typename T>
    T* AddComponent(EntityId id) {
        int componentId = T::Id();
        if (componentId >= _componentPools.size()) {
            _componentPools.resize(componentId + 1);
        }
        ComponentPool& pool = _componentPools[componentId];
        if (!pool.IsInit()) {
            pool.Init(sizeof(T));
        }
        T* pComp = new (pool.Get(id.GetIndex())) T();
        _entities[id.GetIndex()].set(componentId);
        return pComp;
    }

    std::vector<EntityInfo> _entities;
    std::vector<EntityIndex> _freeEntities;
    std::vector<ComponentPool> _componentPools;
};

int main() {

    return 0;
}