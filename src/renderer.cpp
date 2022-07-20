#include "renderer.h"

#include <algorithm>

#include "model.h"
#include "matrix.h"
#include "constants.h"
#include "resource_manager.h"
#include "game_manager.h"
#include "version_id_list.h"

namespace renderer {

Mat4 Camera::GetViewMatrix() const {
    Vec3 p = _transform.GetPos();
    Vec3 forward = -_transform.GetZAxis();  // Z-axis points backward
    Vec3 up = _transform.GetYAxis();
    return Mat4::LookAt(p, p + forward, up);
}

class SceneInternal {
public:
    SceneInternal() :
        _pointLights(100),
        _modelInstances(100) {}
    TVersionIdList<PointLight> _pointLights;
    TVersionIdList<ModelInstance> _modelInstances;
};

Scene::Scene() {
    _pInternal = std::make_unique<SceneInternal>();
};

Scene::~Scene() {}

std::pair<VersionId, PointLight*> Scene::AddPointLight() {
    PointLight* light = nullptr;
    VersionId newId = _pInternal->_pointLights.AddItem(&light);
    return std::make_pair(newId, light);
}
PointLight* Scene::GetPointLight(VersionId id) {
    return _pInternal->_pointLights.GetItem(id);
}
bool Scene::RemovePointLight(VersionId id) {
    return _pInternal->_pointLights.RemoveItem(id);
}

std::pair<VersionId, ModelInstance*> Scene::AddModelInstance() {
    ModelInstance* m = nullptr;
    VersionId newId = _pInternal->_modelInstances.AddItem(&m);
    return std::make_pair(newId, m);
}
ModelInstance* Scene::GetModelInstance(VersionId id) {
    return _pInternal->_modelInstances.GetItem(id);
}
bool Scene::RemoveModelInstance(VersionId id) {
    return _pInternal->_modelInstances.RemoveItem(id);
}

void Scene::Draw(int windowWidth, int windowHeight) {
    assert(_pInternal->_pointLights.GetCount() == 1);
    PointLight const& light = *(_pInternal->_pointLights.GetItemAtIndex(0));

    float aspectRatio = (float)windowWidth / windowHeight;
    Mat4 viewProjTransform = Mat4::Perspective(
        _camera._fovyRad, aspectRatio, /*near=*/_camera._zNear, /*far=*/_camera._zFar);
    Mat4 camMatrix = _camera.GetViewMatrix();
    viewProjTransform = viewProjTransform * camMatrix;

    // TODO: group them by Material
    for (int modelIx = 0; modelIx < _pInternal->_modelInstances.GetCount(); ++modelIx) {
        ModelInstance const* m = _pInternal->_modelInstances.GetItemAtIndex(modelIx);
        m->_mesh->_mat->_shader.Use();
        Mat4 const& transMat = m->_transform;
        Shader const& shader = m->_mesh->_mat->_shader;
        shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
        shader.SetMat4("uModelTrans", transMat);
        shader.SetMat3("uModelInvTrans", transMat.GetMat3());
        shader.SetVec3("uLightPos", light._p);
        shader.SetVec3("uAmbient", light._ambient);
        shader.SetVec3("uDiffuse", light._diffuse);
        shader.SetVec4("uColor", m->_color);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m->_mesh->_mat->_texture->_handle);
        glBindVertexArray(m->_mesh->_vao);
        glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m->_mesh->_numVerts);
    }
}

} // namespace renderer