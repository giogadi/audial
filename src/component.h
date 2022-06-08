#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <iostream>
#include <memory>
#include <sstream>

#include "matrix.h"
#include "game_manager.h"

namespace cereal {
    class access;
}
class Entity;
class EntityManager;

#define M_COMPONENT_TYPES \
    X(Transform) \
    X(Velocity) \
    X(Model) \
    X(Light) \
    X(Camera) \
    X(RigidBody) \
    X(PlayerController) \
    X(BeepOnHit) \
    X(Sequencer) \
    X(PlayerOrbitController) \
    X(CameraController) \
    X(HitCounter) \
    X(Orbitable) \
    X(EventsOnHit) \
    X(Activator) \
    X(Damage) \
    X(OnDestroyEvent)

enum class ComponentType: int {
#   define X(a) a,
    M_COMPONENT_TYPES
#   undef X
    NumTypes
};

extern char const* gComponentTypeStrings[];

// TODO: can make this constexpr?
char const* ComponentTypeToString(ComponentType c);

ComponentType StringToComponentType(char const* s);

bool LoadEntities(char const* filename, bool dieOnConnectFailure, EntityManager& e, GameManager& g);

bool SaveEntities(char const* filename, EntityManager const& e);

void SaveEntity(std::ostream& output, Entity const& e);

void LoadEntity(std::istream& input, Entity& e);

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

// MUST BE PURE or else Cereal won't work. Need at least one = 0 function in there.
class Component {
public:
    virtual ~Component() {}
    virtual void Update(float dt) {}
    // MUST BE PURE or else Cereal won't work
    virtual void Destroy() {};
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) { return true; }
    virtual ComponentType Type() const = 0;
    // If true, request that we try reconnecting the entity's components.
    virtual bool DrawImGui();
    virtual void OnEditPick() {}
    virtual void EditModeUpdate(float dt) {}
};

class Entity {
public:
    // Only finds active components.
    template <typename T>
    std::weak_ptr<T> FindComponentOfType() {
        auto iter = _componentTypeMap.find(std::type_index(typeid(T)));
        if (iter == _componentTypeMap.end()) {
            // std::cout << "Could not find component" << std::endl;
            return std::weak_ptr<T>();
        }
        ComponentAndStatus* compAndStatus = iter->second;
        if (!compAndStatus->_active) {
            return std::weak_ptr<T>();
        }
        std::shared_ptr<T> comp = std::dynamic_pointer_cast<T>(compAndStatus->_c);
        return comp;
    }

    void Update(float dt) {
        for (auto& c : _components) {
            if (c->_active) {
                c->_c->Update(dt);
            }            
        }
    }

    void EditModeUpdate(float dt) {
        for (auto& c : _components) {
            if (c->_active) {
                c->_c->EditModeUpdate(dt);
            }            
        }
    }

    void Destroy() {
        for (auto& c : _components) {
            // TODO: do I need to consider component._active here?
            c->_c->Destroy();
        }
        ResetWithoutComponentDestroy();
    }

    // Used for edit mode to not invoke event logic and such when deleting an entity.
    void ResetWithoutComponentDestroy() {
        _components.clear();
        _componentTypeMap.clear();
    }

    // NOTE: DOES NOT CALL DESTROY ON REMOVED COMPONENT.
    void RemoveComponent(int index) {
        ComponentType compType = _components[index]->_c->Type();
        for (auto item = _componentTypeMap.begin(); item != _componentTypeMap.end(); ++item) {
            if (item->second->_c->Type() == compType) {
                _componentTypeMap.erase(item);
                break;
            }
        }
        // _components[index]->_c->Destroy();
        _components.erase(_components.begin() + index);
    }

    void ConnectComponentsOrDie(EntityId id, GameManager& g) {
        for (auto& compAndStatus : _components) {
            if (compAndStatus->_active) {
                assert(compAndStatus->_c->ConnectComponents(id, *this, g));
            }
        }
    }

    void ConnectComponentsOrDeactivate(
        EntityId id, GameManager& g, std::vector<ComponentType>* failures = nullptr) {
        if (_components.empty()) {
            return;
        }        
        std::vector<bool> active(_components.size());
        for (int i = 0; i < _components.size(); ++i) {
            active[i] = _components[i]->_active;
        }
        bool allActiveSucceeded = false;
        for (int roundIx = 0; !allActiveSucceeded && roundIx < _components.size() + 1; ++roundIx) {
            // Clone components into a parallel vector, delete the original, start over.
            std::stringstream entityData;
            SaveEntity(entityData, *this);
            _components.clear();
            _componentTypeMap.clear();
            LoadEntity(entityData, *this);

            allActiveSucceeded = true;
            for (int i = 0; i < _components.size(); ++i) {
                if (active[i]) {
                    bool success = _components[i]->_c->ConnectComponents(id, *this, g);
                    if (!success) {
                        active[i] = false;
                        allActiveSucceeded = false;
                    }
                }
            }
        }
        assert(allActiveSucceeded);
    }

    std::weak_ptr<Component> TryAddComponentOfType(ComponentType c);

    template<typename T>
    std::weak_ptr<T> TryAddComponent() {
        auto iter = _componentTypeMap.try_emplace(
            std::type_index(typeid(T)));
        if (!iter.second) {
            return std::weak_ptr<T>();
        }

        auto component = std::make_shared<T>();
        {
            auto compAndStatus = std::make_unique<ComponentAndStatus>();
            compAndStatus->_c = component;
            compAndStatus->_active = true;
            _components.push_back(std::move(compAndStatus));
        }

        iter.first->second = _components.back().get();
        return component;
    }

    template<typename T>
    std::weak_ptr<T> AddComponentOrDie() {
        std::weak_ptr<T> pComp = TryAddComponent<T>();
        assert(!pComp.expired());
        return pComp;
    }

    // FOR EDITING AND CEREALIZATION ONLY
    int GetNumComponents() const { return _components.size(); }
    Component const& GetComponent(int i) const { return *_components.at(i)->_c; }
    Component& GetComponent(int i) {        
        return *_components.at(i)->_c;
    }
    void OnEditPickComponents() {
        for (auto& compAndStatus : _components) {
            if (compAndStatus->_active) {
                compAndStatus->_c->OnEditPick();
            }
        }
    }

    std::string _name;
private:
    struct ComponentAndStatus {
        std::shared_ptr<Component> _c;
        bool _active = true;
    };
    // We need this to be a vector of pointers so that _componentTypeMap can
    // have stable pointers to the vector's data even if vector gets
    // reallocated.
    std::vector<std::unique_ptr<ComponentAndStatus>> _components;
    std::unordered_map<std::type_index, ComponentAndStatus*> _componentTypeMap;
    friend class cereal::access;
};

class TransformComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Transform; }
    TransformComponent()
        : _transform(Mat4::Identity()) {}

    virtual ~TransformComponent() {}

    // TODO make these return const& to memory in Mat4
    Vec3 GetPos() const {
        return _transform.GetCol3(3);
    }
    void SetPos(Vec3 const& p) {
        _transform._col3 = Vec4(p._x, p._y, p._z, 1.f);
    }
    Vec3 GetXAxis() const {
        return _transform.GetCol3(0);
    }
    Vec3 GetYAxis() const {
        return _transform.GetCol3(1);
    }
    Vec3 GetZAxis() const {
        return _transform.GetCol3(2);
    }
    Mat4 GetMat4() const {
        return _transform;
    }

    Mat3 GetRot() const {
        return _transform.GetMat3();
    }
    void SetRot(Mat3 const& rot) {
        _transform.SetTopLeftMat3(rot);
    }

    // TODO store separate rot and pos
    Mat4 _transform;
};

class VelocityComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Velocity; }
    VelocityComponent()
        :_linear(0.f,0.f,0.f) {}
    
    virtual void Update(float dt) override {
        std::shared_ptr<TransformComponent> transform = _transform.lock();
        transform->SetPos(transform->GetPos() + dt * _linear);
        Mat3 rot = transform->GetRot();
        rot = Mat3::FromAxisAngle(Vec3(0.f, 1.f, 0.f), dt * _angularY) * rot;
        transform->SetRot(rot);
    }

    virtual void Destroy() override {}

    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override {
        _transform = e.FindComponentOfType<TransformComponent>();
        return !_transform.expired();
    }

    std::weak_ptr<TransformComponent> _transform;
    Vec3 _linear;
    float _angularY = 0.0f;  // rad / s
    
};

enum class EntityDestroyType {
    None, DestroyComponents, ResetWithoutDestroy
};

struct EntityAndStatus {
    std::shared_ptr<Entity> _e;   
    EntityId _id = EntityId::InvalidId(); 
    bool _active = true;
    EntityDestroyType _destroy = EntityDestroyType::None;
};

class EntityManager {
public:
    EntityId AddEntity(bool active=true) {
        EntityAndStatus* e_s;
        if (_freeList.empty()) {
            _entities.emplace_back();
            e_s = &_entities.back();
            e_s->_id = EntityId(_entities.size() - 1, 0);
        } else {
            EntityIndex freeIndex = _freeList.back();
            _freeList.pop_back();
            e_s = &_entities[freeIndex];
            EntityId newId(freeIndex, e_s->_id.GetVersion());
            e_s->_id = newId;
        }
        e_s->_e = std::make_shared<Entity>();
        e_s->_active = active;
        e_s->_destroy = EntityDestroyType::None;
        return e_s->_id;
    }

    // ONLY FOR USE IN EDIT MODE TO MAINTAIN ORDERING OF ENTITIES
    EntityId AddEntityToBack(bool active=true) {
        _entities.emplace_back();
        EntityAndStatus* e_s = &_entities.back();
        e_s->_id = EntityId(_entities.size() - 1, 0);
        e_s->_e = std::make_shared<Entity>();
        e_s->_active = active;
        e_s->_destroy = EntityDestroyType::None;
        return e_s->_id;
    }

    void TagEntityForDestroy(
        EntityId idToDestroy, EntityDestroyType destroyType = EntityDestroyType::DestroyComponents) {
        if (!idToDestroy.IsValid()) {
            return;
        }
        EntityIndex indexToDestroy = idToDestroy.GetIndex();
        if (indexToDestroy >= _entities.size()) {
            return;
        }
        EntityAndStatus& e_s = _entities[indexToDestroy];
        if (e_s._id != idToDestroy) {
            std::cout << "ALREADY DESTROYED THIS DUDE" << std::endl;
            return;
        }
        e_s._destroy = destroyType;
    }

    void Update(float dt) {
        ForEveryActiveEntity([this, dt](EntityId id) {
            this->GetEntity(id)->Update(dt);
        });
    }

    void EditModeUpdate(float dt) {
        ForEveryActiveEntity([this, dt](EntityId id) {
            this->GetEntity(id)->EditModeUpdate(dt);
        });
    }
    
    void ConnectComponents(GameManager& g, bool dieOnConnectFailure) {
        ForEveryActiveEntity([&](EntityId id) {
            Entity* e = this->GetEntity(id);
            if (dieOnConnectFailure) {
                e->ConnectComponentsOrDie(id, g);
            } else {
                e->ConnectComponentsOrDeactivate(id, g, /*failures=*/nullptr);
            }
        });
    }

    EntityId FindActiveEntityByName(char const* name) {
        for (auto const& entity : _entities) {
            if (entity._id.IsValid() && entity._active && entity._e->_name == name) {
                return entity._id;
            }
        }
        return EntityId::InvalidId();
    }

    EntityId FindInactiveEntityByName(char const* name) {
        for (auto const& entity : _entities) {
            if (entity._id.IsValid() && !entity._active && entity._e->_name == name) {
                return entity._id;
            }
        }
        return EntityId::InvalidId();
    }

    void DeactivateEntity(EntityId id) {
        EntityAndStatus* e_s = GetEntityAndStatus(id);
        if (e_s == nullptr) {
            return;
        }
        if (!e_s->_active) {
            return;
        }
        std::stringstream entityData;
        SaveEntity(entityData, *e_s->_e);
        e_s->_e->ResetWithoutComponentDestroy();
        LoadEntity(entityData, *e_s->_e);
        e_s->_active = false;
    }

    void ActivateEntity(EntityId id, GameManager& g) {
        EntityAndStatus* e_s = GetEntityAndStatus(id);
        if (e_s == nullptr) {
            return;
        }
        if (e_s->_active) {
            return;
        }
        e_s->_e->ConnectComponentsOrDeactivate(id, g, /*failures=*/nullptr);
        e_s->_active = true;
    }

    void DestroyTaggedEntities() {
        ForEveryActiveAndInactiveEntity([this](EntityId id) {
            EntityAndStatus& e_s = *this->GetEntityAndStatus(id);
            switch (e_s._destroy) {
                case EntityDestroyType::None:
                    return;
                case EntityDestroyType::DestroyComponents:
                    e_s._e->Destroy();
                    break;
                case EntityDestroyType::ResetWithoutDestroy:
                    e_s._e->ResetWithoutComponentDestroy();
                    break;                    
            }            
            // TODO: We don't need to delete the entity yet, right? It should
            // get automatically deleted when we do AddEntity().
            e_s._id = EntityId(EntityIndex(-1), id.GetVersion() + 1);
            e_s._destroy = EntityDestroyType::None;
            this->_freeList.push_back(id.GetIndex());
        });        
    }

    void ForEveryActiveEntity(std::function<void(EntityId)> f) const {
        for (auto const& e_s : _entities) {
            if (e_s._id.IsValid() && e_s._active) {
                f(e_s._id);
            }
        }
    }
    void ForEveryActiveAndInactiveEntity(std::function<void(EntityId)> f) const {
        for (auto const& e_s : _entities) {
            if (e_s._id.IsValid()) {
                f(e_s._id);
            }
        }
    }

    // DO NOT STORE POINTER!!!
    Entity* GetEntity(EntityId id) {
        EntityAndStatus* e_s = GetEntityAndStatus(id);
        if (e_s == nullptr) {
            return nullptr;
        }
        return e_s->_e.get();
    }
    Entity const* GetEntity(EntityId id) const {
        EntityAndStatus const* e_s = GetEntityAndStatus(id);
        if (e_s == nullptr) {
            return nullptr;
        }
        return e_s->_e.get();
    }

    bool IsActive(EntityId id) const {
        EntityAndStatus const* e_s = GetEntityAndStatus(id);
        if (e_s == nullptr) {
            return false;
        }
        return e_s->_active;
    }

private:
    // DON'T STORE THIS POINTER! Maybe should be edit-only or something
    EntityAndStatus* GetEntityAndStatus(EntityId id) {
        if (id.IsValid()) {
            EntityIndex index = id.GetIndex();
            if (index < _entities.size()) {
                EntityAndStatus& e_s = _entities[index];
                if (e_s._id == id) {
                    return &e_s;
                }                
            }
        }
        return nullptr;
    }
    EntityAndStatus const* GetEntityAndStatus(EntityId id) const {
        if (id.IsValid()) {
            EntityIndex index = id.GetIndex();
            if (index < _entities.size()) {
                EntityAndStatus const& e_s = _entities[index];
                if (e_s._id == id) {
                    return &e_s;
                }                
            }
        }
        return nullptr;
    }

    std::vector<EntityAndStatus> _entities;
    std::vector<EntityIndex> _freeList;
};