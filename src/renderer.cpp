#include "renderer.h"

#include <algorithm>

#include "imgui/imgui.h"

#include "model.h"
#include "input_manager.h"
#include "matrix.h"
#include "constants.h"
#include "resource_manager.h"
#include "serial.h"
#include "entity.h"
#include "game_manager.h"
#include "components/transform.h"
#include "components/model_instance.h"
#include "components/light.h"
#include "components/camera.h"

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
        Mat4 transMat = m._transform.lock()->GetWorldMat4();
        m._mesh->_mat->_shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
        m._mesh->_mat->_shader.SetMat4("uModelTrans", transMat);
        m._mesh->_mat->_shader.SetMat3("uModelInvTrans", transMat.GetMat3());
        m._mesh->_mat->_shader.SetVec3("uLightPos", light->_transform.lock()->GetWorldPos());
        m._mesh->_mat->_shader.SetVec3("uAmbient", light->_ambient);
        m._mesh->_mat->_shader.SetVec3("uDiffuse", light->_diffuse);
        m._mesh->_mat->_shader.SetVec4("uColor", m._color);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m._mesh->_mat->_texture->_handle);
        glBindVertexArray(m._mesh->_vao);
        glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m._mesh->_numVerts);
    }
}