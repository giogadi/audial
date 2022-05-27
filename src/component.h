#pragma once

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
    X(PlayerOrbitController)

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

// MUST BE PURE or else Cereal won't work. Need at least one = 0 function in there.
class Component {
public:
    virtual ~Component() {}
    virtual void Update(float dt) {}
    // MUST BE PURE or else Cereal won't work
    virtual void Destroy() {};
    virtual bool ConnectComponents(Entity& e, GameManager& g) { return true; }
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
            // TODO: do I need to consider _active here?
            c->_c->Destroy();
        }
    }

    void RemoveComponent(int index) {
        ComponentType compType = _components[index]->_c->Type();
        for (auto item = _componentTypeMap.begin(); item != _componentTypeMap.end(); ++item) {
            if (item->second->_c->Type() == compType) {
                _componentTypeMap.erase(item);
                break;
            }
        }
        _components[index]->_c->Destroy();
        _components.erase(_components.begin() + index);
    }

    void ConnectComponentsOrDie(GameManager& g) {
        for (auto& compAndStatus : _components) {
            if (compAndStatus->_active) {
                assert(compAndStatus->_c->ConnectComponents(*this, g));
            }
        }
    }

    void ConnectComponentsOrDeactivate(
        GameManager& g, std::vector<ComponentType>* failures = nullptr) {
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
                    bool success = _components[i]->_c->ConnectComponents(*this, g);
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

    virtual bool ConnectComponents(Entity& e, GameManager& g) override {
        _transform = e.FindComponentOfType<TransformComponent>();
        return !_transform.expired();
    }

    std::weak_ptr<TransformComponent> _transform;
    Vec3 _linear;
    float _angularY = 0.0f;  // rad / s
    
};

class EntityManager {
public:
    std::weak_ptr<Entity> AddEntity() {
        _entities.push_back(std::make_shared<Entity>());
        return _entities.back();
    }

    void DestroyEntity(std::weak_ptr<Entity> weakToDestroy) {
        std::shared_ptr<Entity> toDestroy = weakToDestroy.lock();
        if (toDestroy == nullptr) {
            return;
        }
        for (int i = 0; i < _entities.size(); ++i) {
            if (_entities[i] == toDestroy) {
                _entities[i]->Destroy();
                _entities.erase(_entities.begin() + i);
                break;
            }
        }
    }

    void DestroyEntity(int index) {
        assert(index >= 0 && index < _entities.size());
        _entities[index]->Destroy();
        _entities.erase(_entities.begin() + index);
    }

    void Update(float dt) {
        for (auto& e : _entities) {
            e->Update(dt);
        }
    }

    void EditModeUpdate(float dt) {
        for (auto& e : _entities) {
            e->EditModeUpdate(dt);
        }
    }
    
    void ConnectComponents(GameManager& g, bool dieOnConnectFailure) {
        for (auto& pEntity : _entities) {
            if (dieOnConnectFailure) {
                pEntity->ConnectComponentsOrDie(g);
            } else {
                pEntity->ConnectComponentsOrDeactivate(g, /*failures=*/nullptr);
            }
        }
    }

    std::vector<std::shared_ptr<Entity>> _entities;
};