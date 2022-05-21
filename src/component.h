#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <iostream>
#include <memory>

namespace cereal {
    class access;
}

#include "matrix.h"
#include "game_manager.h"

class Entity;
class TransformManager;

#define M_COMPONENT_TYPES \
    X(Transform) \
    X(Velocity) \
    X(Model) \
    X(Light) \
    X(Camera) \
    X(RigidBody) \
    X(PlayerController) \
    X(BeepOnHit) \
    X(Sequencer)

enum class ComponentType: int {
#   define X(a) a,
    M_COMPONENT_TYPES
#   undef X
    NumTypes
};

// TODO: can make this constexpr?
char const* ComponentTypeToString(ComponentType c);

ComponentType StringToComponentType(char const* s);

// MUST BE PURE or else Cereal won't work. Need at least one = 0 function in there.
class Component {
public:
    virtual ~Component() {}
    virtual void Update(float dt) {}
    // MUST BE PURE or else Cereal won't work
    virtual void Destroy() {};
    virtual void ConnectComponents(Entity& e, GameManager& g) {}
    virtual ComponentType Type() const = 0;
    virtual void DrawImGui();
};

class Entity {
public:
    template <typename T>
    std::weak_ptr<T> FindComponentOfType() {
        auto iter = _componentTypeMap.find(std::type_index(typeid(T)));
        if (iter == _componentTypeMap.end()) {
            // std::cout << "Could not find component" << std::endl;
            return std::weak_ptr<T>();
        }
        std::shared_ptr<T> comp = std::dynamic_pointer_cast<T>(iter->second.lock());
        return comp;
    }

    void Update(float dt) {
        for (auto& c : _components) {
            c->Update(dt);
        }
    }

    void Destroy() {
        for (auto& c : _components) {
            c->Destroy();
        }
    }

    void ConnectComponents(GameManager& g) {
        for (auto& c : _components) {
            c->ConnectComponents(*this, g);
        }
    }

    template<typename T>
    std::weak_ptr<T> AddComponent() {
        std::shared_ptr<T> comp = std::make_shared<T>();
        bool success = _componentTypeMap.emplace(std::type_index(typeid(T)), comp).second;
        assert(success);
        _components.push_back(comp);
        return comp;
    }

    int GetNumComponents() const { return _components.size(); }
    Component const& GetComponent(int i) const { return *_components.at(i); }
    Component& GetComponent(int i) {        
        return *_components.at(i);
    }

    std::string _name;
private:
    std::vector<std::shared_ptr<Component>> _components;
    std::unordered_map<std::type_index, std::weak_ptr<Component>> _componentTypeMap;
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
    TransformManager* _mgr;
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

    virtual void ConnectComponents(Entity& e, GameManager& g) override {
        _transform = e.FindComponentOfType<TransformComponent>();
        assert(!_transform.expired());
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

    void Update(float dt) {
        for (auto& e : _entities) {
            e->Update(dt);
        }
    }

    void ConnectComponents(GameManager& g) {
        for (auto& pEntity : _entities) {
            pEntity->ConnectComponents(g);
        }
    }

    std::vector<std::shared_ptr<Entity>> _entities;
};