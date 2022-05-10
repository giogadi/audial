#pragma once

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include "model.h"

class TransformComponent {
public:
    TransformComponent()
        : _transform(1.f) {}

    glm::vec3 GetPos() const {
        return glm::vec3(_transform[3]);
    }
    void SetPos(glm::vec3 const& p) {
        _transform[3] = glm::vec4(p, 1.f);
    }

    glm::mat4 _transform;
};

class ModelComponent {
public:
    TransformComponent const* _transform = nullptr;
    glm::mat4 const* _viewProjTransform = nullptr;
    BoundMesh const* _mesh = nullptr;

    friend class SceneManager;

private:
    ModelComponent(
        TransformComponent const* trans, glm::mat4 const* viewProjTrans, BoundMesh const* mesh)
        : _transform(trans)
        , _viewProjTransform(viewProjTrans)
        , _mesh(mesh)
    {}
};

class LightComponent {
public:
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

class SceneManager {
public:
    SceneManager(DebugCamera const* camera)
        : _camera(camera) {}
    
    ModelComponent* AddModel(
        TransformComponent const* trans, glm::mat4 const* viewProjTrans, BoundMesh const* mesh) {

        std::unique_ptr<ModelComponent> model(new ModelComponent(
            trans, viewProjTrans, mesh));
        _models.push_back(std::move(model));
        return _models.back().get();
    }

    LightComponent* SetLight(
        TransformComponent const* trans, glm::vec3 const& ambient, glm::vec3 const& diffuse) {
            
        _light.reset(new LightComponent(trans, ambient, diffuse));
        return _light.get();
    }

    void Draw() {
        // TODO: group them by Material
        // TODO: compute viewProjTransform from camera instead of storing it in model component
        // TODO: make Camera a component?
        for (auto const& pModel : _models) {
            ModelComponent const& m = *pModel;
            m._mesh->_mat->_shader.Use();
            m._mesh->_mat->_shader.SetMat4("uMvpTrans", *m._viewProjTransform * m._transform->_transform);
            m._mesh->_mat->_shader.SetMat4("uModelTrans", m._transform->_transform);
            m._mesh->_mat->_shader.SetMat3("uModelInvTrans", glm::mat3(glm::inverseTranspose(m._transform->_transform)));
            assert(_light != nullptr);
            m._mesh->_mat->_shader.SetVec3("uLightPos", _light->_transform->GetPos());
            m._mesh->_mat->_shader.SetVec3("uAmbient", _light->_ambient);
            m._mesh->_mat->_shader.SetVec3("uDiffuse", _light->_diffuse);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m._mesh->_mat->_texture->_handle);
            glBindVertexArray(m._mesh->_vao);
            glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m._mesh->_numVerts);
        }
    }

    std::vector<std::unique_ptr<ModelComponent>> _models;
    std::unique_ptr<LightComponent> _light;
    DebugCamera const* _camera = nullptr;  
};

class Entity {
public:
    Entity() {}

    Entity(std::unique_ptr<TransformComponent> t, ModelComponent* m)
        : _transform(std::move(t))
        , _model(m)
    {}

    std::unique_ptr<TransformComponent> _transform;
    ModelComponent* _model;
};