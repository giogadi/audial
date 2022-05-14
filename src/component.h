#pragma once

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"

#include "matrix.h"

class Component {
public:
    virtual ~Component() {}
    virtual void Update(float dt) {}
    virtual void Destroy() {}
};

class TransformComponent : public Component {
public:
    TransformComponent()
        : _transform(Mat4::Identity()) {}

    virtual ~TransformComponent() {}

    glm::vec3 GetPos() const {
        return glm::vec3(_transform._m03, _transform._m13, _transform._m23);
    }
    void SetPos(glm::vec3 const& p) {
        _transform._col3 = Vec4(p.x, p.y, p.z, 1.f);
    }
    Vec3 GetPosNew() const {
        return _transform.GetCol3(3);
    }
    void SetPosNew(Vec3 const& p) {
        _transform._col3 = Vec4(p._x, p._y, p._z, 1.f);
    }

    // TODO make these return const& to memory in Mat4
    glm::vec3 GetXAxis() const {
        Vec3 c = _transform.GetCol3(0);
        return glm::vec3(c._x, c._y, c._z);
    }
    glm::vec3 GetYAxis() const {
        Vec3 c = _transform.GetCol3(1);
        return glm::vec3(c._x, c._y, c._z);
    }
    glm::vec3 GetZAxis() const {
        Vec3 c = _transform.GetCol3(2);
        return glm::vec3(c._x, c._y, c._z);
    }

    Vec3 GetXAxisNew() const {
        return _transform.GetCol3(0);
    }
    Vec3 GetYAxisNew() const {
        return _transform.GetCol3(1);
    }
    Vec3 GetZAxisNew() const {
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
    VelocityComponent(TransformComponent* t)
        : _transform(t)
        , _linear(0.f,0.f,0.f) {}
    
    virtual void Update(float dt) override {
        _transform->SetPosNew(_transform->GetPosNew() + dt * _linear);
        Mat3 rot = _transform->GetRot();
        rot = Mat3::FromAxisAngle(Vec3(0.f, 1.f, 0.f), dt * _angularY) * rot;
        _transform->SetRot(rot);
        // glm::mat4 t = TEMP_ToGlmMat4(_transform->_transform);
        // _transform->_transform = TEMP_ToMat4(glm::rotate(t, dt * _angularY, glm::vec3(0.f,1.f,0.f)));
    }

    TransformComponent* _transform = nullptr;
    Vec3 _linear;
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