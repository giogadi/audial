#pragma once

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include "model.h"
#include "input_manager.h"

class Component {
public:
    virtual ~Component() {}
};

class TransformComponent : public Component {
public:
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

private:
    TransformComponent()
        : _transform(1.f) {}

    friend class TransformManager;
};

class TransformManager {
public:
    TransformComponent* CreateTransform() {
        std::unique_ptr<TransformComponent> t(new TransformComponent());
        _transforms.push_back(std::move(t));
        return _transforms.back().get();
    }

    // TODO Obviously make these not pointers
    std::vector<std::unique_ptr<TransformComponent>> _transforms;
};

class ModelComponent : public Component {
public:
    virtual ~ModelComponent() {}

    TransformComponent const* _transform = nullptr;
    BoundMesh const* _mesh = nullptr;

private:
    ModelComponent(
        TransformComponent const* trans, BoundMesh const* mesh)
        : _transform(trans)
        , _mesh(mesh)
    {}

    friend class SceneManager;
};

class LightComponent : public Component {
public:
    virtual ~LightComponent() {}

    TransformComponent const* _transform;
    glm::vec3 _ambient;
    glm::vec3 _diffuse;

private:
    LightComponent(TransformComponent const* t, glm::vec3 const& ambient, glm::vec3 const& diffuse)
        : _transform(t) 
        , _ambient(ambient)
        , _diffuse(diffuse) {}
    friend class SceneManager;
};

class CameraComponent : public Component {
public:    
    void Init(TransformComponent* t, InputManager const* input) {
        _transform = t;
        _input = input;
    }

    virtual ~CameraComponent() {}

    glm::mat4 GetViewMatrix() const {
        glm::vec3 p = _transform->GetPos();
        glm::vec3 forward = -_transform->GetZAxis();  // Z-axis points backward
        glm::vec3 up = _transform->GetYAxis();
        return glm::lookAt(p, p + forward, up);
    }

    // TODO: make movement relative to camera viewpoint
    // TODO: break movement out into its own component
    void Update(float const dt) {
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

private:
    CameraComponent() {}
    CameraComponent(TransformComponent* t, InputManager const* input)
        : _transform(t)
        , _input(input) {}
    
    friend class SceneManager;
};

class SceneManager {
public:    
    void Update(float dt) {
        assert(_cameras.size() == 1);
        _cameras.front()->Update(dt);
    }

    ModelComponent* AddModel(TransformComponent* t, BoundMesh const* mesh) {
        std::unique_ptr<ModelComponent> model(new ModelComponent(
            t, mesh));
        _models.push_back(std::move(model));
        return _models.back().get();
    }

    LightComponent* AddLight(
        TransformComponent* t, glm::vec3 const& ambient, glm::vec3 const& diffuse) {
        std::unique_ptr<LightComponent> light(new LightComponent(t, ambient, diffuse));
        _lights.push_back(std::move(light));
        return _lights.back().get();
    }

    CameraComponent* AddCamera(TransformComponent* t, InputManager const* input) {
        std::unique_ptr<CameraComponent> camera(new CameraComponent(t, input));
        _cameras.push_back(std::move(camera));
        return _cameras.back().get();
    }

    void Draw(int windowWidth, int windowHeight) {
        assert(_cameras.size() == 1);
        assert(_lights.size() == 1);
        CameraComponent const* camera = _cameras.front().get();
        LightComponent const* light = _lights.front().get();

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

    // TODO: are these std::unique_ptr's safe when resizing the vector?
    std::vector<std::unique_ptr<ModelComponent>> _models;
    std::vector<std::unique_ptr<LightComponent>> _lights;
    std::vector<std::unique_ptr<CameraComponent>> _cameras;
};

class Entity {
public:
    template <typename T>
    T* DebugFindComponentOfType() {
        for (Component* c : _components) {
            T* t = dynamic_cast<T*>(c);
            if (t != nullptr) {
                return t;
            }
        }
        return nullptr;
    }

    std::vector<Component*> _components;

private:
    Entity() {}

    friend class EntityManager;
};

class EntityManager {
public:
    Entity* AddEntity() {
        {
            std::unique_ptr<Entity> e(new Entity);
            _entities.push_back(std::move(e));
        }
        return _entities.back().get();
    }

    std::vector<std::unique_ptr<Entity>> _entities;
};