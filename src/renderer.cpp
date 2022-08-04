#include "renderer.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>

#include "stb_image.h"
#include <glad/glad.h>

#include "mesh.h"
#include "shader.h"
#include "matrix.h"
#include "constants.h"
#include "version_id_list.h"
#include "cube_verts.h"

namespace renderer {

Mat4 Camera::GetViewMatrix() const {
    Vec3 p = _transform.GetPos();
    Vec3 forward = -_transform.GetZAxis();  // Z-axis points backward
    Vec3 up = _transform.GetYAxis();
    return Mat4::LookAt(p, p + forward, up);
}

namespace {
bool CreateTextureFromFile(char const* filename, unsigned int& textureId) {
    stbi_set_flip_vertically_on_load(true);
    int texWidth, texHeight, texNumChannels;
    unsigned char *texData = stbi_load(filename, &texWidth, &texHeight, &texNumChannels, 0);
    if (texData == nullptr) {
        std::cout << "Error: could not load texture " << filename << std::endl;
        return false;
    }

    glGenTextures(1, &textureId);
    // TODO: Do I need this here, or only in rendering?
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D, /*mipmapLevel=*/0, /*textureFormat=*/GL_RGB, texWidth, texHeight, /*legacy=*/0,
        /*sourceFormat=*/GL_RGB, /*sourceDataType=*/GL_UNSIGNED_BYTE, texData);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(texData);

    return true;
}
}

class SceneInternal {
public:
    SceneInternal() :
        _pointLights(100),
        _colorModelInstances(100),
        _texturedModelInstances(100),
        _topLayerColorModels(100) {
            _modelsToDraw.reserve(100);
    }
    bool Init();
    TVersionIdList<PointLight> _pointLights;
    TVersionIdList<ColorModelInstance> _colorModelInstances;
    TVersionIdList<TexturedModelInstance> _texturedModelInstances;
    std::vector<ColorModelInstance> _modelsToDraw;

    std::unordered_map<std::string, std::unique_ptr<BoundMeshPNU>> _meshMap;
    std::unordered_map<std::string, unsigned int> _textureIdMap;
    Shader _colorShader;
    Shader _textureShader;

    // boring cached things
    std::vector<ColorModelInstance const*> _topLayerColorModels;
    BoundMeshPNU const* _cubeMesh = nullptr;
};

bool SceneInternal::Init() {    
    {
        std::array<float,kCubeVertsNumValues> cubeVerts;
        GetCubeVertices(&cubeVerts);
        int const numCubeVerts = 36;
        auto mesh = std::make_unique<BoundMeshPNU>();
        mesh->Init(cubeVerts.data(), numCubeVerts);
        assert(_meshMap.emplace("cube", std::move(mesh)).second);

        mesh = std::make_unique<BoundMeshPNU>();
        assert(mesh->Init("data/models/axes.obj"));
        assert(_meshMap.emplace("axes", std::move(mesh)).second);
    }

    if (!_colorShader.Init("shaders/shader.vert", "shaders/color.frag")) {
        return false;
    }

    if (!_textureShader.Init("shaders/shader.vert", "shaders/shader.frag")) {
        return false;
    }

    unsigned int woodboxTextureId = 0;
    if (!CreateTextureFromFile("data/textures/wood_container.jpg", woodboxTextureId)) {
        return false;
    }
    _textureIdMap.emplace("wood_box", woodboxTextureId);

    _cubeMesh = _meshMap["cube"].get();
    return true;
}

Scene::Scene() {
    _pInternal = std::make_unique<SceneInternal>();
};

Scene::~Scene() {}

bool Scene::Init() {
    return _pInternal->Init();
}

BoundMeshPNU const* Scene::GetMesh(std::string const& meshName) const {
    auto meshIter = _pInternal->_meshMap.find(meshName);
    if (meshIter == _pInternal->_meshMap.end()) {
        return nullptr;
    } else {
        return meshIter->second.get();
    }
}

renderer::ColorModelInstance& Scene::DrawMesh() {
    _pInternal->_modelsToDraw.emplace_back();
    return _pInternal->_modelsToDraw.back();
}

void Scene::DrawMesh(BoundMeshPNU const* mesh, Mat4 const& t, Vec4 const& color) {
    ColorModelInstance& model = DrawMesh();
    model._transform = t;
    model._mesh = mesh;
    model._color = color;
}

void Scene::DrawCube(Mat4 const& t, Vec4 const& color) {
    DrawMesh(_pInternal->_cubeMesh, t, color);
}

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

std::pair<VersionId, ColorModelInstance*> Scene::AddColorModelInstance() {
    ColorModelInstance* m = nullptr;
    VersionId newId = _pInternal->_colorModelInstances.AddItem(&m);
    return std::make_pair(newId, m);
}
ColorModelInstance* Scene::GetColorModelInstance(VersionId id) {
    return _pInternal->_colorModelInstances.GetItem(id);
}
bool Scene::RemoveColorModelInstance(VersionId id) {
    return _pInternal->_colorModelInstances.RemoveItem(id);
}

std::pair<VersionId, TexturedModelInstance*> Scene::AddTexturedModelInstance() {
    TexturedModelInstance* m = nullptr;
    VersionId newId = _pInternal->_texturedModelInstances.AddItem(&m);
    return std::make_pair(newId, m);
}
TexturedModelInstance* Scene::GetTexturedModelInstance(VersionId id) {
    return _pInternal->_texturedModelInstances.GetItem(id);
}
bool Scene::RemoveTexturedModelInstance(VersionId id) {
    return _pInternal->_texturedModelInstances.RemoveItem(id);
}

void Scene::Draw(int windowWidth, int windowHeight) {    
    assert(_pInternal->_pointLights.GetCount() == 1);
    PointLight const& light = *(_pInternal->_pointLights.GetItemAtIndex(0));

    float aspectRatio = (float)windowWidth / windowHeight;
    Mat4 viewProjTransform = Mat4::Perspective(
        _camera._fovyRad, aspectRatio, /*near=*/_camera._zNear, /*far=*/_camera._zFar);
    Mat4 camMatrix = _camera.GetViewMatrix();
    viewProjTransform = viewProjTransform * camMatrix;

    auto& topLayerModels = _pInternal->_topLayerColorModels;
    topLayerModels.clear();

    // buffered Color models (immediate API)
    {
        Shader& shader = _pInternal->_colorShader;
        shader.Use();
        shader.SetVec3("uLightPos", light._p);
        shader.SetVec3("uAmbient", light._ambient);
        shader.SetVec3("uDiffuse", light._diffuse);
        for (ColorModelInstance const& m : _pInternal->_modelsToDraw) {
            if (!m._visible) {
                continue;
            }
            if (m._topLayer) {
                topLayerModels.push_back(&m);
            }
            Mat4 const& transMat = m._transform;
            shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
            shader.SetMat4("uModelTrans", transMat);
            Mat3 modelTransInv;
            assert(transMat.GetMat3().TransposeInverse(modelTransInv));
            shader.SetMat3("uModelInvTrans", modelTransInv);

            // TEMPORARY! What we really want is to use the submeshes always unless the outer Model
            // has defined some overrides.
            if (m._mesh->_subMeshes.size() == 0) {
                shader.SetVec4("uColor", m._color);
                glBindVertexArray(m._mesh->_vao);
                glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m._mesh->_numVerts);
            } else {
                for (int subMeshIx = 0; subMeshIx < m._mesh->_subMeshes.size(); ++subMeshIx) {
                    BoundMeshPNU::SubMesh const& subMesh = m._mesh->_subMeshes[subMeshIx];
                    shader.SetVec4("uColor", subMesh._color);
                    glBindVertexArray(m._mesh->_vao);
                    glDrawArrays(GL_TRIANGLES, subMesh._startIndex, subMesh._numVerts);
                }
            }
        }

        _pInternal->_modelsToDraw.clear();
    }    

    // Color models
    {
        Shader& shader = _pInternal->_colorShader;
        shader.Use();
        shader.SetVec3("uLightPos", light._p);
        shader.SetVec3("uAmbient", light._ambient);
        shader.SetVec3("uDiffuse", light._diffuse);
        for (int modelIx = 0; modelIx < _pInternal->_colorModelInstances.GetCount(); ++modelIx) {
            ColorModelInstance const* m = _pInternal->_colorModelInstances.GetItemAtIndex(modelIx);
            if (!m->_visible) {
                continue;
            }
            if (m->_topLayer) {
                topLayerModels.push_back(m);
            }
            Mat4 const& transMat = m->_transform;
            shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
            shader.SetMat4("uModelTrans", transMat);
            Mat3 modelTransInv;
            assert(transMat.GetMat3().TransposeInverse(modelTransInv));
            shader.SetMat3("uModelInvTrans", modelTransInv);

            // TEMPORARY! What we really want is to use the submeshes always unless the outer Model
            // has defined some overrides.
            if (m->_mesh->_subMeshes.size() == 0) {
                shader.SetVec4("uColor", m->_color);
                glBindVertexArray(m->_mesh->_vao);
                glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m->_mesh->_numVerts);
            } else {
                for (int subMeshIx = 0; subMeshIx < m->_mesh->_subMeshes.size(); ++subMeshIx) {
                    BoundMeshPNU::SubMesh const& subMesh = m->_mesh->_subMeshes[subMeshIx];
                    shader.SetVec4("uColor", subMesh._color);
                    glBindVertexArray(m->_mesh->_vao);
                    glDrawArrays(GL_TRIANGLES, subMesh._startIndex, subMesh._numVerts);
                }
            }
        }
    }    

    // Textured models
    {
        Shader& shader = _pInternal->_textureShader;
        shader.Use();
        shader.SetVec3("uLightPos", light._p);
        shader.SetVec3("uAmbient", light._ambient);
        shader.SetVec3("uDiffuse", light._diffuse);
        for (int modelIx = 0; modelIx < _pInternal->_texturedModelInstances.GetCount(); ++modelIx) {
            TexturedModelInstance const* m = _pInternal->_texturedModelInstances.GetItemAtIndex(modelIx);
            Mat4 const& transMat = m->_transform;
            shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
            shader.SetMat4("uModelTrans", transMat);
            Mat3 modelTransInv;
            assert(transMat.GetMat3().TransposeInverse(modelTransInv));
            shader.SetMat3("uModelInvTrans", modelTransInv);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m->_textureId);
            glBindVertexArray(m->_mesh->_vao);
            glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m->_mesh->_numVerts);
        }
    }

    // TODO: maybe we should move all glClear()'s into renderer.cpp and save
    // game.cpp from including any GL code?
    glClear(GL_DEPTH_BUFFER_BIT);

    // Top layer color models.
    // TODO prolly refactor both color draws into a common function
    // Color models
    {
        Shader& shader = _pInternal->_colorShader;
        shader.Use();
        shader.SetVec3("uLightPos", light._p);
        shader.SetVec3("uAmbient", light._ambient);
        shader.SetVec3("uDiffuse", light._diffuse);
        for (ColorModelInstance const* m : _pInternal->_topLayerColorModels) {
            if (!m->_visible) {
                continue;
            }
            Mat4 const& transMat = m->_transform;
            shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
            shader.SetMat4("uModelTrans", transMat);
            Mat3 modelTransInv;
            assert(transMat.GetMat3().TransposeInverse(modelTransInv));
            shader.SetMat3("uModelInvTrans", modelTransInv);

            // TEMPORARY! What we really want is to use the submeshes always unless the outer Model
            // has defined some overrides.
            if (m->_mesh->_subMeshes.size() == 0) {
                shader.SetVec4("uColor", m->_color);
                glBindVertexArray(m->_mesh->_vao);
                glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, m->_mesh->_numVerts);
            } else {
                for (int subMeshIx = 0; subMeshIx < m->_mesh->_subMeshes.size(); ++subMeshIx) {
                    BoundMeshPNU::SubMesh const& subMesh = m->_mesh->_subMeshes[subMeshIx];
                    shader.SetVec4("uColor", subMesh._color);
                    glBindVertexArray(m->_mesh->_vao);
                    glDrawArrays(GL_TRIANGLES, subMesh._startIndex, subMesh._numVerts);
                }
            }
        }
    }  
}

} // namespace renderer