#pragma once

#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"

class TransformComponent {
public:
    TransformComponent()
        : _transform(1.f) {
        _transform = glm::rotate(_transform, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
        _transform = glm::rotate(_transform, glm::radians(45.f), glm::vec3(1.f, 0.f, 0.f));
    }

    void Update() {
        // DEBUG
        float dt = 0.016f;
        _transform = glm::rotate(
            _transform, dt * glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 _transform;
};

class ModelComponent {
public:
    ModelComponent(
        TransformComponent const* trans, glm::mat4 const* viewProjTrans, Shader const& shader, 
        int texture, int vao, int numVertices)
        : _transform(trans)
        , _viewProjTransform(viewProjTrans)
        , _shader(shader)
        , _textureHandle(texture)
        , _vao(vao)
        , _numVertices(numVertices)
    {}


    void Update() {
        _shader.Use();
        _shader.SetMat4("transform", *_viewProjTransform * _transform->_transform);
        glBindTexture(GL_TEXTURE_2D, _textureHandle);
        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, _numVertices);
    }

    TransformComponent const* _transform = nullptr;
    glm::mat4 const* _viewProjTransform = nullptr;
    Shader _shader;
    int _textureHandle = -1;
    int _vao = -1;
    int _numVertices = -1;
};

class Entity {
public:
    Entity() {}

    Entity(std::unique_ptr<TransformComponent> t, std::unique_ptr<ModelComponent> m)
        : _transform(std::move(t))
        , _model(std::move(m))
    {}

    void Update(float dt) {
        _transform->Update();
        _model->Update();
    }

    std::unique_ptr<TransformComponent> _transform;
    std::unique_ptr<ModelComponent>_model;
};