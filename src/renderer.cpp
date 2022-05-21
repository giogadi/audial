#include "renderer.h"

#include <algorithm>

#include "imgui/imgui.h"

#include "model.h"
#include "input_manager.h"
#include "matrix.h"
#include "constants.h"
#include "resource_manager.h"

void ModelComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    ModelManager* modelManager = g._modelManager;
    _mesh = modelManager->_modelMap.at(_modelId).get();
    _mgr = g._sceneManager;
    _mgr->AddModel(e.FindComponentOfType<ModelComponent>());
}

void LightComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    _mgr = g._sceneManager;
    _mgr->AddLight(e.FindComponentOfType<LightComponent>());
}

void LightComponent::DrawImGui() {
    ImGui::InputScalar("Diffuse R", ImGuiDataType_Float, &_diffuse._x, /*step=*/nullptr);
    ImGui::InputScalar("Diffuse G", ImGuiDataType_Float, &_diffuse._y, /*step=*/nullptr);
    ImGui::InputScalar("Diffuse B", ImGuiDataType_Float, &_diffuse._z, /*step=*/nullptr);

    ImGui::InputScalar("Ambient R", ImGuiDataType_Float, &_ambient._x, /*step=*/nullptr);
    ImGui::InputScalar("Ambient G", ImGuiDataType_Float, &_ambient._y, /*step=*/nullptr);
    ImGui::InputScalar("Ambient B", ImGuiDataType_Float, &_ambient._z, /*step=*/nullptr);
}

void CameraComponent::ConnectComponents(Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    _input = g._inputManager;
    _mgr = g._sceneManager;
    _mgr->AddCamera(e.FindComponentOfType<CameraComponent>());
}

Mat4 CameraComponent::GetViewMatrix() const {
    auto transform = _transform.lock();
    Vec3 p = transform->GetPos();
    Vec3 forward = -transform->GetZAxis();  // Z-axis points backward
    Vec3 up = transform->GetYAxis();
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
    auto transform = _transform.lock();
    Vec3 newPos = transform->GetPos() + translation;
    transform->SetPos(newPos);
}

void SceneManager::Draw(int windowWidth, int windowHeight) {
    // Update our lists of models, cameras, and lights if any of them were deleted.
    _models.erase(std::remove_if(_models.begin(), _models.end(),
        [](std::weak_ptr<ModelComponent const> const& p) {
            return p.expired();
        }), _models.end());
    _cameras.erase(std::remove_if(_cameras.begin(), _cameras.end(),
        [](std::weak_ptr<CameraComponent const> const& p) {
            return p.expired();
        }), _cameras.end());
    _lights.erase(std::remove_if(_lights.begin(), _lights.end(),
        [](std::weak_ptr<LightComponent const> const& p) {
            return p.expired();
        }), _lights.end());
    

    assert(_cameras.size() == 1);
    assert(_lights.size() == 1);
    auto const camera = _cameras.front().lock();
    auto const light = _lights.front().lock();

    float fovy = 45.f * kPi / 180.f;
    float aspectRatio = (float)windowWidth / windowHeight;
    Mat4 viewProjTransform = Mat4::Perspective(
        fovy, aspectRatio, /*near=*/0.1f, /*far=*/100.f);
    Mat4 camMatrix = camera->GetViewMatrix();
    viewProjTransform = viewProjTransform * camMatrix;

    // TODO: group them by Material
    for (auto const& pModel : _models) {
        auto const m = *pModel.lock();
        m._mesh->_mat->_shader.Use();
        Mat4 transMat = m._transform.lock()->GetMat4();
        m._mesh->_mat->_shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
        m._mesh->_mat->_shader.SetMat4("uModelTrans", transMat);
        m._mesh->_mat->_shader.SetMat3("uModelInvTrans", transMat.GetMat3());
        m._mesh->_mat->_shader.SetVec3("uLightPos", light->_transform.lock()->GetPos());
        m._mesh->_mat->_shader.SetVec3("uAmbient", light->_ambient);
        m._mesh->_mat->_shader.SetVec3("uDiffuse", light->_diffuse);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m._mesh->_mat->_texture->_handle);
        glBindVertexArray(m._mesh->_vao);
        glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m._mesh->_numVerts);
    }
}