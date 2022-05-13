#include "renderer.h"

#include <algorithm>

#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/ext/matrix_clip_space.hpp"

#include "model.h"
#include "input_manager.h"

ModelComponent::ModelComponent(TransformComponent const* trans, BoundMesh const* mesh, SceneManager* sceneMgr)
        : _transform(trans)
        , _mesh(mesh)
        , _mgr(sceneMgr) {
    _mgr->AddModel(this);
}

void ModelComponent::Destroy() {
    _mgr->RemoveModel(this);
}

LightComponent::LightComponent(TransformComponent const* t, glm::vec3 const& ambient, glm::vec3 const& diffuse, SceneManager* sceneMgr)
        : _transform(t) 
        , _ambient(ambient)
        , _diffuse(diffuse)
        , _mgr(sceneMgr) {
    _mgr->AddLight(this);
}

void LightComponent::Destroy() {
    _mgr->RemoveLight(this);
}

CameraComponent::CameraComponent(TransformComponent* t, InputManager const* input, SceneManager* sceneMgr)
        : _transform(t)
        , _input(input)
        , _mgr(sceneMgr) {
    _mgr->AddCamera(this);
}

void CameraComponent::Destroy() {
    _mgr->RemoveCamera(this);
}

glm::mat4 CameraComponent::GetViewMatrix() const {
    glm::vec3 p = _transform->GetPos();
    glm::vec3 forward = -_transform->GetZAxis();  // Z-axis points backward
    glm::vec3 up = _transform->GetYAxis();
    return glm::lookAt(p, p + forward, up);
}

void CameraComponent::Update(float dt) {
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

void SceneManager::RemoveModel(ModelComponent const* m) { 
    _models.erase(std::remove(_models.begin(), _models.end(), m));
}
void SceneManager::RemoveLight(LightComponent const* l) {
    _lights.erase(std::remove(_lights.begin(), _lights.end(), l));
}
void SceneManager::RemoveCamera(CameraComponent const* c) { 
    _cameras.erase(std::remove(_cameras.begin(), _cameras.end(), c));
}
void SceneManager::Draw(int windowWidth, int windowHeight) {
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