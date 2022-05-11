#pragma once

#include <vector>
#include <algorithm>

#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include "model.h"
#include "input_manager.h"

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

class SceneManager;

class ModelComponent : public Component {
public:
    ModelComponent(TransformComponent const* trans, BoundMesh const* mesh, SceneManager* sceneMgr);
    virtual ~ModelComponent() {}
    virtual void Destroy() override;

    TransformComponent const* _transform = nullptr;
    BoundMesh const* _mesh = nullptr;
    SceneManager* _mgr = nullptr;
};

class LightComponent : public Component {
public:
    LightComponent(TransformComponent const* t, glm::vec3 const& ambient, glm::vec3 const& diffuse, SceneManager* sceneMgr);
    virtual ~LightComponent() {}
    virtual void Destroy() override;

    TransformComponent const* _transform;
    glm::vec3 _ambient;
    glm::vec3 _diffuse;
    SceneManager* _mgr;
};

class CameraComponent : public Component {
public:  
    CameraComponent(TransformComponent* t, InputManager const* input, SceneManager* sceneMgr);

    virtual ~CameraComponent() {}
    virtual void Destroy() override;

    glm::mat4 GetViewMatrix() const {
        glm::vec3 p = _transform->GetPos();
        glm::vec3 forward = -_transform->GetZAxis();  // Z-axis points backward
        glm::vec3 up = _transform->GetYAxis();
        return glm::lookAt(p, p + forward, up);
    }

    // TODO: make movement relative to camera viewpoint
    // TODO: break movement out into its own component
    virtual void Update(float const dt) override {
        return;

        glm::vec3 inputVec(0.0f);
        if (_input->IsKeyPressed(InputManager::Key::W)) {
            inputVec.z -= 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::S)) {
            inputVec.z += 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::A)) {
            inputVec.x -= 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::D)) {
            inputVec.x += 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::Q)) {
            inputVec.y += 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::E)) {
            inputVec.y -= 1.0f;
        }

        if (inputVec.x == 0.f && inputVec.y == 0.f && inputVec.z == 0.f) {
            return;
        }

        float const kSpeed = 5.0f;
        glm::vec3 translation = dt * kSpeed * glm::normalize(inputVec);
        glm::vec3 newPos = _transform->GetPos() + translation;
        _transform->SetPos(newPos);
    }

    TransformComponent* _transform = nullptr;
    InputManager const* _input = nullptr;
    SceneManager* _mgr = nullptr;
};

class SceneManager {
public:    
    void AddModel(ModelComponent const* m) { _models.push_back(m); }
    void AddLight(LightComponent const* l) { _lights.push_back(l); }
    void AddCamera(CameraComponent const* c) { _cameras.push_back(c); }

    void RemoveModel(ModelComponent const* m) { 
        _models.erase(std::remove(_models.begin(), _models.end(), m));
    }
    void RemoveLight(LightComponent const* l) {
        _lights.erase(std::remove(_lights.begin(), _lights.end(), l));
    }
    void RemoveCamera(CameraComponent const* c) { 
        _cameras.erase(std::remove(_cameras.begin(), _cameras.end(), c));
    }

    void Draw(int windowWidth, int windowHeight) {
        assert(_cameras.size() == 1);
        assert(_lights.size() == 1);
        CameraComponent const* camera = _cameras.front();
        LightComponent const* light = _lights.front();

        float aspectRatio = (float)windowWidth / windowHeight;
        glm::mat4 viewProjTransform = glm::perspective(
            /*fovy=*/glm::radians(45.f), aspectRatio, /*near=*/0.1f, /*far=*/100.0f);
        viewProjTransform = viewProjTransform * camera->GetViewMatrix();

        // TODO: group them by Material
        for (auto const& pModel : _models) {
            ModelComponent const& m = *pModel;
            m._mesh->_mat->_shader.Use();
            m._mesh->_mat->_shader.SetMat4("uMvpTrans", viewProjTransform * m._transform->_transform);
            m._mesh->_mat->_shader.SetMat4("uModelTrans", m._transform->_transform);
            m._mesh->_mat->_shader.SetMat3("uModelInvTrans", glm::mat3(glm::inverseTranspose(m._transform->_transform)));
            m._mesh->_mat->_shader.SetVec3("uLightPos", light->_transform->GetPos());
            m._mesh->_mat->_shader.SetVec3("uAmbient", light->_ambient);
            m._mesh->_mat->_shader.SetVec3("uDiffuse", light->_diffuse);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m._mesh->_mat->_texture->_handle);
            glBindVertexArray(m._mesh->_vao);
            glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m._mesh->_numVerts);
        }
    }

    std::vector<ModelComponent const*> _models;
    std::vector<LightComponent const*> _lights;
    std::vector<CameraComponent const*> _cameras;
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