#pragma once

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"

class Component {
public:
    virtual ~Component() {}
    virtual void Update(float dt) {}
    virtual void Destroy() {}
};

class TransformComponent : public Component {
public:
    TransformComponent()
        : _transform(1.f) {}

    virtual ~TransformComponent() {}

    glm::vec3 GetPos() const {
        return glm::vec3(_transform[3]);
    }
    void SetPos(glm::vec3 const& p) {
        _transform[3] = glm::vec4(p, 1.f);
    }

    glm::vec3 GetXAxis() const {
        return glm::vec3(_transform[0]);
    }
    glm::vec3 GetYAxis() const {
        return glm::vec3(_transform[1]);
    }
    glm::vec3 GetZAxis() const {
        return glm::vec3(_transform[2]);
    }

    glm::mat4 _transform;
};

class VelocityComponent : public Component {
public:
    VelocityComponent(TransformComponent* t)
        : _transform(t)
        , _linear(0.f) {}
    
    virtual void Update(float dt) override {
        _transform->SetPos(_transform->GetPos() + dt * _linear);
        _transform->_transform = glm::rotate(_transform->_transform, dt * _angularY, glm::vec3(0.f,1.f,0.f));
    }

    TransformComponent* _transform = nullptr;
    glm::vec3 _linear;
    float _angularY = 0.0f;  // rad / s
};

class Entity {
public:
    template <typename T>
    T* DebugFindComponentOfType() {
        for (std::unique_ptr<Component>& c : _components) {
            T* t = dynamic_cast<T*>(c.get());
            if (t != nullptr) {
                return t;
            }
        }
        return nullptr;
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

    std::vector<std::unique_ptr<Component>> _components;
};

class EntityManager {
public:
    Entity* AddEntity() {
        _entities.push_back(std::make_unique<Entity>());
        return _entities.back().get();
    }
    void DestroyEntity(Entity* toDestroy) {
        for (int i = 0; i < _entities.size(); ++i) {
            if (_entities[i].get() == toDestroy) {
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

    std::vector<std::unique_ptr<Entity>> _entities;
};