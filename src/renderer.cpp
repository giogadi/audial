#include "renderer.h"

#include <algorithm>

#include "model.h"
#include "input_manager.h"
#include "matrix.h"
#include "constants.h"
#include "resource_manager.h"

void ModelComponent::Destroy() {
    _mgr->RemoveModel(this);
}

void ModelComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.DebugFindComponentOfType<TransformComponent>();
    ModelManager* modelManager = g._modelManager;
    _mesh = modelManager->_modelMap.at(_modelId).get();
    _mgr = g._sceneManager;
    _mgr->AddModel(this);
}

void LightComponent::Destroy() {
    _mgr->RemoveLight(this);
}

void LightComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.DebugFindComponentOfType<TransformComponent>();
    _mgr = g._sceneManager;
    _mgr->AddLight(this);
}

void CameraComponent::Destroy() {
    _mgr->RemoveCamera(this);
}

void CameraComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.DebugFindComponentOfType<TransformComponent>();
    _input = g._inputManager;
    _mgr = g._sceneManager;
    _mgr->AddCamera(this);
}

Mat4 CameraComponent::GetViewMatrix() const {
    Vec3 p = _transform->GetPos();
    Vec3 forward = -_transform->GetZAxis();  // Z-axis points backward
    Vec3 up = _transform->GetYAxis();
    return Mat4::LookAt(p, p + forward, up);
}

void CameraComponent::Update(float dt) {
    return;

    Vec3 inputVec(0.f,0.f,0.f);
    if (_input->IsKeyPressed(InputManager::Key::W)) {
        inputVec._z -= 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::S)) {
        inputVec._z += 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::A)) {
        inputVec._x -= 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::D)) {
        inputVec._x += 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::Q)) {
        inputVec._y += 1.0f;
    }
    if (_input->IsKeyPressed(InputManager::Key::E)) {
        inputVec._y -= 1.0f;
    }

    if (inputVec._x == 0.f && inputVec._y == 0.f && inputVec._z == 0.f) {
        return;
    }

    float const kSpeed = 5.0f;
    Vec3 translation = dt * kSpeed * inputVec.GetNormalized();
    Vec3 newPos = _transform->GetPos() + translation;
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

    float fovy = 45.f * kPi / 180.f;
    float aspectRatio = (float)windowWidth / windowHeight;
    Mat4 viewProjTransform = Mat4::Perspective(
        fovy, aspectRatio, /*near=*/0.1f, /*far=*/100.f);
    Mat4 camMatrix = camera->GetViewMatrix();
    viewProjTransform = viewProjTransform * camMatrix;

    // TODO: group them by Material
    for (auto const& pModel : _models) {
        ModelComponent const& m = *pModel;
        m._mesh->_mat->_shader.Use();
        Mat4 transMat = m._transform->GetMat4();
        m._mesh->_mat->_shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
        m._mesh->_mat->_shader.SetMat4("uModelTrans", transMat);
        m._mesh->_mat->_shader.SetMat3("uModelInvTrans", transMat.GetMat3());
        m._mesh->_mat->_shader.SetVec3("uLightPos", light->_transform->GetPos());
        m._mesh->_mat->_shader.SetVec3("uAmbient", light->_ambient);
        m._mesh->_mat->_shader.SetVec3("uDiffuse", light->_diffuse);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m._mesh->_mat->_texture->_handle);
        glBindVertexArray(m._mesh->_vao);
        glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m._mesh->_numVerts);
    }
}