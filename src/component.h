#pragma once

#include <vector>

#include "matrix.h"

class Entity;

class Component {
public:
    virtual ~Component() {}
    virtual void Update(float dt) {}
    // MUST BE PURE or else Cereal won't work
    virtual void Destroy() = 0;
    virtual void ConnectComponents(Entity& e) {}
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

    void ConnectComponents() {
        for (auto& c : _components) {
            c->ConnectComponents(*this);
        }
    }

    std::vector<std::unique_ptr<Component>> _components;
};

class TransformComponent : public Component {
public:
    TransformComponent()
        : _transform(Mat4::Identity()) {}

    virtual ~TransformComponent() {}
    virtual void Destroy() override {}

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
    VelocityComponent()
        :_linear(0.f,0.f,0.f) {}
    VelocityComponent(TransformComponent* t)
        : _transform(t)
        , _linear(0.f,0.f,0.f) {}
    
    virtual void Update(float dt) override {
        _transform->SetPos(_transform->GetPos() + dt * _linear);
        Mat3 rot = _transform->GetRot();
        rot = Mat3::FromAxisAngle(Vec3(0.f, 1.f, 0.f), dt * _angularY) * rot;
        _transform->SetRot(rot);
    }

    virtual void Destroy() override {}

    virtual void ConnectComponents(Entity& e) override {
        _transform = e.DebugFindComponentOfType<TransformComponent>();
        assert(_transform != nullptr);
    }

    TransformComponent* _transform = nullptr;
    Vec3 _linear;
    float _angularY = 0.0f;  // rad / s
    
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

    void ConnectComponents() {
        for (auto& pEntity : _entities) {
            pEntity->ConnectComponents();
        }
    }

    std::vector<std::unique_ptr<Entity>> _entities;
};