#include "renderer.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <fstream>

#include "stb_image.h"
#include "stb_truetype.h"
#include <glad/glad.h>

#include "mesh.h"
#include "shader.h"
#include "matrix.h"
#include "constants.h"
#include "version_id_list.h"
#include "cube_verts.h"
#include "game_manager.h"

#define DRAW_WATER 0
#define DRAW_TERRAIN 1

namespace renderer {

Mat4 Camera::GetViewMatrix() const {
    Vec3 p = _transform.GetPos();
    Vec3 forward = -_transform.GetCol3(2);  // Z-axis points backward
    Vec3 up = _transform.GetCol3(1);
    return Mat4::LookAt(p, p + forward, up);
}

namespace {
bool CreateTextureFromFile(char const* filename, unsigned int& textureId, bool flip=true) {
    stbi_set_flip_vertically_on_load(flip);
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
    GLenum format;
    switch (texNumChannels) {
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default: assert(false); break;
    }
    glTexImage2D(
        GL_TEXTURE_2D, /*mipmapLevel=*/0, /*textureFormat=*/GL_RGB, texWidth, texHeight, /*legacy=*/0,
        /*sourceFormat=*/format, /*sourceDataType=*/GL_UNSIGNED_BYTE, texData);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(texData);

    return true;
}

bool CreateFontTextureFromBitmap(char const* filename, unsigned int& textureId) {
    int texWidth, texHeight, texNumChannels;
    stbi_set_flip_vertically_on_load(false);
    int desiredChannels = 1;
    unsigned char *texData = stbi_load(filename, &texWidth, &texHeight, &texNumChannels, desiredChannels);
    if (texData == nullptr) {
        std::cout << "Error: could not load texture " << filename << std::endl;
        return false;
    }

    glGenTextures(1, &textureId);
    // TODO: Do I need this here, or only in rendering?
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D, /*mipmapLevel=*/0, /*textureFormat=*/GL_RED, texWidth, texHeight, /*legacy=*/0,
        /*sourceFormat=*/GL_RED, /*sourceDataType=*/GL_UNSIGNED_BYTE, texData);
    stbi_image_free(texData);

    return true;    
}

struct TextInstance {
    std::string _text;
    float _screenX = -1;
    float _screenY = -1;
    Vec4 _colorRgba = Vec4(1.f, 1.f, 1.f, 1.f);
    float _scale = 1.f;
};

struct GlyphInstance {
    Vec4 _colorRgba;
    stbtt_aligned_quad _quad;
};

namespace ColorShaderUniforms {
    enum Names {
        uMvpTrans,
        uModelTrans,
        uModelInvTrans,
        uColor,
        uDirLightDir,
        uDirLightAmb,
        uDirLightDif,
        uPointLight0Pos,
        uPointLight0Amb,
        uPointLight0Dif,
        uPointLight1Pos,
        uPointLight1Amb,
        uPointLight1Dif,
        Count
    };
    static char const* NameStrings[] = {
        "uMvpTrans",
        "uModelTrans",
        "uModelInvTrans",
        "uColor",
        "uDirLight._dir",
        "uDirLight._ambient",
        "uDirLight._diffuse",
        "uPointLights[0]._pos",
        "uPointLights[0]._ambient",
        "uPointLights[0]._diffuse",
        "uPointLights[1]._pos",
        "uPointLights[1]._ambient",
        "uPointLights[1]._diffuse"
    };
};

#if DRAW_WATER
namespace WaterShaderUniforms {
    enum Names {
        uMvpTrans,
        uModelTrans,
        uColor,
        uDirLightDir,
        uDirLightAmb,
        uDirLightDif,
        uTime,
        Count
    };
    static char const* NameStrings[] = {
        "uMvpTrans",
        "uModelTrans",
        "uColor",
        "uDirLight._dir",
        "uDirLight._ambient",
        "uDirLight._diffuse",
        "uTime"
    };
};
#endif

#if DRAW_TERRAIN
namespace TerrainShaderUniforms {
    enum Names {
        uMvpTrans,
        uModelTrans,
        uModelInvTrans,
        uColor,
        uDirLightDir,
        uDirLightAmb,
        uDirLightDif,
        uOffset,
        Count
    };
    static char const* NameStrings[] = {
        "uMvpTrans",
        "uModelTrans",
        "uModelInvTrans",
        "uColor",
        "uDirLight._dir",
        "uDirLight._ambient",
        "uDirLight._diffuse",
        "uOffset"
    };
};
#endif

namespace WireframeShaderUniforms {
    enum Names {
        uMvpTrans,
        uColor,
        Count
    };
    static char const* NameStrings[] = {
        "uMvpTrans",
        "uColor"
    };
}

struct BoundingBoxInstance {
    Mat4 _t;
    Vec4 _color;
};

struct Polygon2dInstance {
    Mat4 _t;
    Vec4 _color;
    BoundMeshPNU const* _mesh = nullptr;
};

}  // namespace

class SceneInternal {
public:
    SceneInternal() :
        _pointLights(100),
        _colorModelInstances(100),
        _texturedModelInstances(100),
        _topLayerColorModels(100) {
        
        _modelsToDraw.reserve(100);
    }
    bool Init(GameManager& g);

    DirLight _dirLight;
    TVersionIdList<PointLight> _pointLights;
    TVersionIdList<ColorModelInstance> _colorModelInstances;
    TVersionIdList<TexturedModelInstance> _texturedModelInstances;
    std::vector<ColorModelInstance> _modelsToDraw;
    std::vector<TextInstance> _textsToDraw;
    std::vector<GlyphInstance> _glyphsToDraw;
    std::vector<BoundingBoxInstance> _boundingBoxesToDraw;
    std::vector<Polygon2dInstance> _polygonsToDraw;

    std::unordered_map<std::string, std::unique_ptr<BoundMeshPNU>> _meshMap;    
    std::unordered_map<std::string, unsigned int> _textureIdMap;
    std::vector<std::pair<Scene::MeshId, std::unique_ptr<BoundMeshPNU>>> _dynLoadedMeshes;
    int _nextMeshId = 0;

    BoundMeshPB _cubeWireframeMesh;
    
    Shader _colorShader;
    int _colorShaderUniforms[ColorShaderUniforms::Count];
    
    Shader _textureShader;

#if DRAW_WATER    
    Shader _waterShader;
    int _waterShaderUniforms[WaterShaderUniforms::Count];
#endif    

#if DRAW_TERRAIN    
    Shader _terrainShader;
    int _terrainShaderUniforms[TerrainShaderUniforms::Count];
#endif    
    
    Shader _textShader;

    unsigned int _textVao = 0;
    unsigned int _textVbo = 0;
    std::vector<stbtt_bakedchar> _fontCharInfo;

    Shader _wireframeShader;
    int _wireframeShaderUniforms[WireframeShaderUniforms::Count];

    

    // boring cached things
    std::vector<ColorModelInstance const*> _topLayerColorModels;
    std::vector<ColorModelInstance const*> _transparentModels;
    BoundMeshPNU const* _cubeMesh = nullptr;

    GameManager* _g = nullptr;
};

namespace {
std::unique_ptr<BoundMeshPNU> MakeWaterMesh() {
    // x points across (+x) and z points down (+z), each point spaced a unit apart
    // Should be indexed row-major, with +x direction being the rows.
    // origin of plane is top-left corner.
    // int numPointsX = 50;
    // int numPointsZ = 40;
    // float spacing = 0.5f;
    int numPointsX = 2;
    int numPointsZ = 2;
    float spacing = 25.f;

    assert(numPointsX >= 2);
    assert(numPointsZ >= 2);

    std::vector<float> vertexData;
    int const numVerts = numPointsX * numPointsZ;
    int const vertexDataSize = numVerts * BoundMeshPNU::kNumValuesPerVertex;
    vertexData.reserve(vertexDataSize);
    float zPos = 0.f;
    for (int z = 0; z < numPointsZ; ++z) {
        float xPos = 0.f;
        for (int x = 0; x < numPointsX; ++x) {
            // Pos
            vertexData.push_back(xPos);
            vertexData.push_back(0.f);
            vertexData.push_back(zPos);

            // Normal
            vertexData.push_back(0.f);
            vertexData.push_back(1.f);
            vertexData.push_back(0.f);

            // UV
            float u = (float) x / (float) (numPointsX - 1);
            float v = 1.f - ((float) z / (float) (numPointsZ - 1));
            vertexData.push_back(u);
            vertexData.push_back(v);

            xPos += spacing;
        }
        zPos += spacing;
    }
    assert(vertexData.size() == vertexDataSize);

    // Index time.
    std::vector<uint32_t> indices;
    int const indicesSize = (numPointsX - 1) * (numPointsZ - 1) * 2 * 3;
    indices.reserve(indicesSize);
    for (int z = 0; z < numPointsZ - 1; ++z) {
        for (int x = 0; x < numPointsX - 1; ++x) {
            uint32_t startIndex = z * numPointsX + x;
            uint32_t start10 = startIndex + numPointsX;
            uint32_t start01 = startIndex + 1;
            uint32_t start11 = start10 + 1;
            // Tri 1
            indices.push_back(startIndex);
            indices.push_back(start10);
            indices.push_back(start01);
            // Tri 2
            indices.push_back(start01);
            indices.push_back(start10);
            indices.push_back(start11);
        }
    }
    assert(indices.size() == indicesSize);

    auto mesh = std::make_unique<BoundMeshPNU>();
    mesh->Init(vertexData.data(), numVerts, indices.data(), indicesSize);
    return mesh;
}
}

bool SceneInternal::Init(GameManager& g) {
    _g = &g;

    _dirLight.Set(/*dir=*/Vec3(0.f, -1.f, 0.f), /*amb=*/Vec3(), /*dif=*/Vec3());    

    // More OpenGL stuff
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    {
        std::array<float,kCubeVertsNumValues> cubeVerts;
        GetCubeVertices(&cubeVerts);
        int const numCubeVerts = 36;
        auto mesh = std::make_unique<BoundMeshPNU>();
        mesh->Init(cubeVerts.data(), numCubeVerts);
        bool success = _meshMap.emplace("cube", std::move(mesh)).second;
        assert(success);

        mesh = std::make_unique<BoundMeshPNU>();
        success = mesh->Init("data/models/axes.obj");
        assert(success);
        success = _meshMap.emplace("axes", std::move(mesh)).second;
        assert(success);

        mesh = std::make_unique<BoundMeshPNU>();
        success = mesh->Init("data/models/cross.obj");
        assert(success);
        success = _meshMap.emplace("cross", std::move(mesh)).second;
        assert(success);

        mesh = MakeWaterMesh();
        success = _meshMap.emplace("water", std::move(mesh)).second;
        assert(success);
    }

    {
        bool success = _cubeWireframeMesh.Init("data/models/cube.obj");
        if (!success) {
            return false;
        }
    }

    if (!_colorShader.Init("shaders/shader.vert", "shaders/color.frag")) {
        return false;
    }
    for (int i = 0; i < ColorShaderUniforms::Count; ++i) {
        _colorShaderUniforms[i] = _colorShader.GetUniformLocation(ColorShaderUniforms::NameStrings[i]);
    }

    if (!_textureShader.Init("shaders/shader.vert", "shaders/shader.frag")) {
        return false;
    }

#if DRAW_WATER    
    if (!_waterShader.Init("shaders/water.vert", "shaders/water.frag")) {
        return false;
    }
    for (int i = 0; i < WaterShaderUniforms::Count; ++i) {
        _waterShaderUniforms[i] = _waterShader.GetUniformLocation(WaterShaderUniforms::NameStrings[i]);
    }
#endif

#if DRAW_TERRAIN    
    if (!_terrainShader.Init("shaders/terrain.vert", "shaders/terrain.frag")) {
        return false;
    }
    for (int i = 0; i < TerrainShaderUniforms::Count; ++i) {
        _terrainShaderUniforms[i] = _terrainShader.GetUniformLocation(TerrainShaderUniforms::NameStrings[i]);
    }
#endif

    if (!_textShader.Init("shaders/text.vert", "shaders/text.frag")) {
        return false;
    }

    if (!_wireframeShader.Init("shaders/wireframe.vert", "shaders/wireframe.frag", "shaders/wireframe.geom")) {
        return false;
    }
    for (int i = 0; i < WireframeShaderUniforms::Count; ++i) {
        _wireframeShaderUniforms[i] = _wireframeShader.GetUniformLocation(WireframeShaderUniforms::NameStrings[i]);
    }

    unsigned int woodboxTextureId = 0;
    if (!CreateTextureFromFile("data/textures/wood_container.jpg", woodboxTextureId)) {
        return false;
    }
    _textureIdMap.emplace("wood_box", woodboxTextureId);

    unsigned int perlinNoiseTextureId = 0;
    if (!CreateTextureFromFile("data/textures/perlin_noise.png", perlinNoiseTextureId)) {
        return false;
    }
    _textureIdMap.emplace("perlin_noise", perlinNoiseTextureId);

    unsigned int perlinNoiseNormalTextureId = 0;
    if (!CreateTextureFromFile("data/textures/perlin_noise_normal.png", perlinNoiseNormalTextureId)) {
        return false;
    }
    _textureIdMap.emplace("perlin_noise_normal", perlinNoiseNormalTextureId);

    unsigned int fontTextureId = 0;
    if (!CreateFontTextureFromBitmap("data/fonts/videotype/videotype.bmp", fontTextureId)) {
        return false;
    }
    _textureIdMap.emplace("font", fontTextureId);

    {
        // read in font info
        std::ifstream fontInfo("data/fonts/videotype/videotype.info");
        if (!fontInfo.is_open()) {
            printf("Couldn't find font info file!\n");
            return false;
        }
        while (!fontInfo.eof()) {
            std::string line;
            std::getline(fontInfo, line);
            if (line == "") {
                continue;
            }
            std::stringstream lineStream(line);
            stbtt_bakedchar charInfo;
            lineStream >> charInfo.x0 >> charInfo.y0 >> charInfo.x1 >> charInfo.y1 >> charInfo.xoff >> charInfo.yoff >> charInfo.xadvance;
            _fontCharInfo.push_back(charInfo);
        }
        assert(_fontCharInfo.size() == 58);
    }

    // DO THE FONT VAO/VBO HERE
    glGenVertexArrays(1, &_textVao);
    glGenBuffers(1, &_textVbo);
    glBindVertexArray(_textVao);
    glBindBuffer(GL_ARRAY_BUFFER, _textVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    _cubeMesh = _meshMap["cube"].get();
    return true;
}

Scene::Scene() {
    _pInternal = std::make_unique<SceneInternal>();
};

Scene::~Scene() {}

bool Scene::Init(GameManager& g) {
    return _pInternal->Init(g);
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

renderer::ColorModelInstance* Scene::DrawMesh(MeshId id) {
    for (auto const& entry : _pInternal->_dynLoadedMeshes) {
        if (entry.first._id == id._id) {
            renderer::ColorModelInstance* model = &(_pInternal->_modelsToDraw.emplace_back());
            model->_mesh = entry.second.get();
            return model;
        }
    }
    return nullptr;
}

renderer::ColorModelInstance& Scene::DrawMesh(BoundMeshPNU const* mesh, Mat4 const& t, Vec4 const& color) {
    ColorModelInstance& model = DrawMesh();
    model._transform = t;
    model._mesh = mesh;
    model._color = color;
    model._useMeshColor = false;
    return model;
}

renderer::ColorModelInstance& Scene::DrawCube(Mat4 const& t, Vec4 const& color) {
    return DrawMesh(_pInternal->_cubeMesh, t, color);
}

void Scene::DrawBoundingBox(Mat4 const& t, Vec4 const& color) {
    _pInternal->_boundingBoxesToDraw.emplace_back();
    BoundingBoxInstance& bb = _pInternal->_boundingBoxesToDraw.back();
    bb._t = t;
    bb._color = color;
}

void Scene::DrawText(std::string_view str, float& screenX, float& screenY, float scale, Vec4 const& colorRgba) {
    // Pretend the "reference" size is with viewport resolution of size 2560/1494 with screen=pixels
    ViewportInfo const& vp = _pInternal->_g->_viewportInfo;
    scale *= ((float)vp._height / 1494.f);
    // TODO: for some reason we don't need this? Would be nice to fully figure out why, lol
    // account for screen units not being the same as pixels.
    // scale *= ((float)vp._width / (float) vp._screenWidth);
    
    Vec3 origin(screenX, screenY, 0.f);
    for (char c : str) {
        char constexpr kFirstChar = 65;  // 'A'
        int constexpr kNumChars = 58;
        int constexpr kBmpWidth = 512;
        int constexpr kBmpHeight = 128;
        int charIndex = c - kFirstChar;
        if (charIndex >= kNumChars || charIndex < 0) {
            printf("ERROR: character \'%c\' not in font!\n", c);
            continue;
        }
        stbtt_aligned_quad quad;
        Vec3 outPos = origin;
        stbtt_GetBakedQuad(_pInternal->_fontCharInfo.data(), kBmpWidth, kBmpHeight, charIndex, &outPos._x, &outPos._y, &quad, /*opengl_fillrule=*/1);
        
        _pInternal->_glyphsToDraw.emplace_back();
        GlyphInstance& t = _pInternal->_glyphsToDraw.back();
        t._colorRgba = colorRgba;
        t._quad.x0 = origin._x + scale * (quad.x0 - origin._x);
        t._quad.y0 = origin._y + scale * (quad.y0 - origin._y);
        t._quad.x1 = t._quad.x0 + scale * (quad.x1 - quad.x0);
        t._quad.y1 = t._quad.y0 + scale * (quad.y1 - quad.y0);
        t._quad.s0 = quad.s0;
        t._quad.s1 = quad.s1;
        t._quad.t0 = quad.t0;
        t._quad.t1 = quad.t1;

        origin += scale * (outPos - origin);
    }

    screenX = origin._x;
    screenY = origin._y;
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

Scene::MeshId Scene::LoadPolygon2d(std::vector<Vec3> const& points) {
    if (points.size() < 3) {
        return MeshId();
    }
    _pInternal->_dynLoadedMeshes.emplace_back();
    auto& entry = _pInternal->_dynLoadedMeshes.back();
    entry.first._id = _pInternal->_nextMeshId++;

    Vec3 centroid(0.f, 0.f, 0.f);
    for (Vec3 const& p : points) {
        centroid += p;
    }
    centroid = centroid / points.size();

    std::vector<float> vertexData;
    vertexData.reserve((points.size() + 1) * BoundMeshPNU::kNumValuesPerVertex);

    std::vector<uint32_t> indexData;
    indexData.reserve(points.size() + 2);

    vertexData.push_back(centroid._x);
    vertexData.push_back(centroid._y);
    vertexData.push_back(centroid._z);

    vertexData.push_back(0.f);
    vertexData.push_back(1.f);
    vertexData.push_back(0.f);

    vertexData.push_back(0.f);
    vertexData.push_back(0.f);

    indexData.push_back(0);
    
    for (Vec3 const& p : points) {
        vertexData.push_back(p._x);
        vertexData.push_back(p._y);
        vertexData.push_back(p._z);

        vertexData.push_back(0.f);
        vertexData.push_back(1.f);
        vertexData.push_back(0.f);

        vertexData.push_back(0.f);
        vertexData.push_back(0.f);

        indexData.push_back(indexData.size());
    }

    // One more index to close the loop
    indexData.push_back(1);

    entry.second = std::make_unique<BoundMeshPNU>();
    entry.second->Init(vertexData.data(), points.size() + 1, indexData.data(), indexData.size());
    entry.second->_useTriangleFan = true;

    return entry.first;
}

Mat4 Scene::GetViewProjTransform() const {
    float aspectRatio = _pInternal->_g->_aspectRatio;
    Mat4 viewProjTransform;
    switch (_camera._projectionType) {
        case Camera::ProjectionType::Perspective:
            viewProjTransform = Mat4::Perspective(
                _camera._fovyRad, aspectRatio, /*near=*/_camera._zNear, /*far=*/_camera._zFar);
            break;
        case Camera::ProjectionType::Orthographic:
            viewProjTransform = Mat4::Ortho(/*width=*/_camera._width, aspectRatio, _camera._zNear, _camera._zFar);
            break;
    }
    Mat4 camMatrix = _camera.GetViewMatrix();
    viewProjTransform = viewProjTransform * camMatrix;
    return viewProjTransform;
}

namespace {
void SetLightUniformsColorShader(SceneInternal& sceneInternal) {
    int const* uniforms = sceneInternal._colorShaderUniforms;
    Shader& shader = sceneInternal._colorShader;
    shader.SetVec3(uniforms[ColorShaderUniforms::uDirLightDir], sceneInternal._dirLight._dir);
    shader.SetVec3(uniforms[ColorShaderUniforms::uDirLightAmb], sceneInternal._dirLight._ambient);
    shader.SetVec3(uniforms[ColorShaderUniforms::uDirLightDif], sceneInternal._dirLight._diffuse);
    int constexpr kMaxNumPointLights = 2;
    if (sceneInternal._pointLights.GetCount() > kMaxNumPointLights) {
        printf("RENDERER ERROR: TOO MANY POINT LIGHTS (%d)\n", sceneInternal._pointLights.GetCount());
    }
    for (int i = 0; i < kMaxNumPointLights; ++i) {
        int posLoc = uniforms[ColorShaderUniforms::uPointLight0Pos + 3*i];
        int ambLoc = uniforms[ColorShaderUniforms::uPointLight0Amb + 3*i];
        int difLoc = uniforms[ColorShaderUniforms::uPointLight0Dif + 3*i];
        if (i >= sceneInternal._pointLights.GetCount()) {
            shader.SetVec3(posLoc, Vec3(0.f, 0.f, 0.f));
            shader.SetVec3(ambLoc, Vec3(0.f, 0.f, 0.f));
            shader.SetVec3(difLoc, Vec3(0.f, 0.f, 0.f));
        } else {
            PointLight const& pl = *sceneInternal._pointLights.GetItemAtIndex(i);
            shader.SetVec3(posLoc, pl._p);
            shader.SetVec3(ambLoc, pl._ambient);
            shader.SetVec3(difLoc, pl._diffuse);
        }
    }
}

#if DRAW_WATER
void SetLightUniformsWaterShader(SceneInternal& sceneInternal) {
    int const* uniforms = sceneInternal._waterShaderUniforms;
    Shader& shader = sceneInternal._waterShader;
    shader.SetVec3(uniforms[WaterShaderUniforms::uDirLightDir], sceneInternal._dirLight._dir);
    shader.SetVec3(uniforms[WaterShaderUniforms::uDirLightAmb], sceneInternal._dirLight._ambient);
    shader.SetVec3(uniforms[WaterShaderUniforms::uDirLightDif], sceneInternal._dirLight._diffuse);
}
#endif

#if DRAW_TERRAIN
void SetLightUniformsTerrainShader(SceneInternal& sceneInternal) {
    int const* uniforms = sceneInternal._terrainShaderUniforms;
    Shader& shader = sceneInternal._terrainShader;
    shader.SetVec3(uniforms[TerrainShaderUniforms::uDirLightDir], sceneInternal._dirLight._dir);
    shader.SetVec3(uniforms[TerrainShaderUniforms::uDirLightAmb], sceneInternal._dirLight._ambient);
    shader.SetVec3(uniforms[TerrainShaderUniforms::uDirLightDif], sceneInternal._dirLight._diffuse);
}
#endif

void DrawColorModelInstance(SceneInternal& internal, Mat4 const& viewProjTransform, ColorModelInstance const& m) {
    if (!m._visible) {
        return;
    }
    if (m._topLayer) {
        internal._topLayerColorModels.push_back(&m);
        return;
    }
    if (m._color._w < 1.f) {
        internal._transparentModels.push_back(&m);
        return;
    }
    Mat4 const& transMat = m._transform;
    internal._colorShader.SetMat4(internal._colorShaderUniforms[ColorShaderUniforms::uMvpTrans], viewProjTransform * transMat);
    internal._colorShader.SetMat4(internal._colorShaderUniforms[ColorShaderUniforms::uModelTrans], transMat);
    Mat3 modelTransInv;
    bool success = transMat.GetMat3().TransposeInverse(modelTransInv);
    assert(success);
    internal._colorShader.SetMat3(internal._colorShaderUniforms[ColorShaderUniforms::uModelInvTrans], modelTransInv);

    // TEMPORARY! What we really want is to use the submeshes always unless the outer Model
    // has defined some overrides.
    assert(m._mesh != nullptr);
    if (m._mesh->_subMeshes.size() == 0) {
        internal._colorShader.SetVec4(internal._colorShaderUniforms[ColorShaderUniforms::uColor], m._color);
        glBindVertexArray(m._mesh->_vao);
        GLenum mode = GL_TRIANGLES;
        if (m._mesh->_useTriangleFan) {
            mode = GL_TRIANGLE_FAN;
        }
        glDrawElements(mode, /*count=*/m._mesh->_numIndices, GL_UNSIGNED_INT, /*start_offset=*/0);
    } else {
        if (!m._useMeshColor) {
            internal._colorShader.SetVec4(internal._colorShaderUniforms[ColorShaderUniforms::uColor], m._color);
        }
        for (int subMeshIx = 0; subMeshIx < m._mesh->_subMeshes.size(); ++subMeshIx) {
            BoundMeshPNU::SubMesh const& subMesh = m._mesh->_subMeshes[subMeshIx];
            if (m._useMeshColor) {
                internal._colorShader.SetVec4(internal._colorShaderUniforms[ColorShaderUniforms::uColor], m._color);
            }
            glBindVertexArray(m._mesh->_vao);
            glDrawElements(GL_TRIANGLES, /*count=*/subMesh._numIndices, GL_UNSIGNED_INT,
                           /*start_offset=*/(void*)(sizeof(uint32_t) * subMesh._startIndex));
        }
    }
}
}

void Scene::Draw(int windowWidth, int windowHeight, float timeInSecs) {
    // TODO migrate the other shaders to multiple lights UGH.
    // assert(_pInternal->_pointLights.GetCount() == 1);
    // PointLight const& light = *(_pInternal->_pointLights.GetItemAtIndex(0));

    // TODO LOAD THE DIR LIGHT IN FROM DATA
    {
        DirLight& dirLight = _pInternal->_dirLight;
        // make it point 45-degrees up from straight-down, up toward -z
        Vec3 dir(0.f, -1.f, -1.f);
        dir.Normalize();
        dirLight.Set(dir, Vec3(0.1f, 0.1f, 0.1f), Vec3(1.f, 1.f, 1.f));
    }

    Mat4 viewProjTransform = GetViewProjTransform();

    auto& topLayerModels = _pInternal->_topLayerColorModels;
    topLayerModels.clear();

    auto& transparentModels = _pInternal->_transparentModels;
    transparentModels.clear();    

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // WATER
#if DRAW_WATER
    {
        Mat4 const transMat;
        Mat3 modelTransInv;
        assert(transMat.GetMat3().TransposeInverse(modelTransInv));

        Shader& shader = _pInternal->_waterShader;
        shader.Use();
        SetLightUniformsWaterShader(*_pInternal);        
        shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
        shader.SetMat4("uModelTrans", transMat);
        shader.SetMat3("uModelInvTrans", modelTransInv);
        shader.SetVec4("uColor", Vec4(0.f, 0.f, 1.f, 1.f));
        shader.SetFloat("uTime", timeInSecs);
        BoundMeshPNU const* waterMesh = GetMesh("water");
        glBindVertexArray(waterMesh->_vao);
        glDrawElements(GL_TRIANGLES, /*count=*/waterMesh->_numIndices, GL_UNSIGNED_INT, /*start_offset=*/0);
    }
#endif

    // TERRAIN
#if DRAW_TERRAIN   
    {
        Mat4 transMat;
        Vec3 cameraPos = _camera._transform.GetPos();
        Vec3 cameraPosToTerrainPos(-10.f, -15.f, -8.f);
        transMat.SetTranslation(cameraPos + cameraPosToTerrainPos);
        Mat3 modelTransInv;
        assert(transMat.GetMat3().TransposeInverse(modelTransInv));

        Shader& shader = _pInternal->_terrainShader;
        shader.Use();
        SetLightUniformsTerrainShader(*_pInternal);  
        shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
        shader.SetMat4("uModelTrans", transMat);
        shader.SetMat3("uModelInvTrans", modelTransInv);
        shader.SetVec4("uColor", Vec4(1.f, 1.f, 1.f, 1.f));
        Vec3 offset(cameraPos._x, 0.f, cameraPos._z);
        shader.SetVec3("uOffset", offset);
        shader.SetInt("uNormalMap", 0);
        unsigned int textureId = _pInternal->_textureIdMap.at("perlin_noise_normal");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        shader.SetInt("uHeightMap", 1);
        textureId = _pInternal->_textureIdMap.at("perlin_noise");
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureId);
        BoundMeshPNU const* waterMesh = GetMesh("water");
        glBindVertexArray(waterMesh->_vao);
        glDrawElements(GL_TRIANGLES, /*count=*/waterMesh->_numIndices, GL_UNSIGNED_INT, /*start_offset=*/0);
    }
#endif

    glClear(GL_DEPTH_BUFFER_BIT);

    // buffered Color models (immediate API)
    {
        Shader& shader = _pInternal->_colorShader;
        shader.Use();
        SetLightUniformsColorShader(*_pInternal);
        for (ColorModelInstance const& m : _pInternal->_modelsToDraw) {
            DrawColorModelInstance(*_pInternal, viewProjTransform, m);
        }

        _pInternal->_modelsToDraw.clear();
    }    

    // Color models
    {
        Shader& shader = _pInternal->_colorShader;
        shader.Use();
        SetLightUniformsColorShader(*_pInternal);
        for (int modelIx = 0; modelIx < _pInternal->_colorModelInstances.GetCount(); ++modelIx) {
            ColorModelInstance const* m = _pInternal->_colorModelInstances.GetItemAtIndex(modelIx);
            DrawColorModelInstance(*_pInternal, viewProjTransform, *m);
        }
    }

    // Polygons (immediate API)
    {
        Shader& shader = _pInternal->_colorShader;
        shader.Use();
        SetLightUniformsColorShader(*_pInternal);
        for (Polygon2dInstance const& poly : _pInternal->_polygonsToDraw) {
            _pInternal->_colorShader.SetMat4(_pInternal->_colorShaderUniforms[ColorShaderUniforms::uMvpTrans], viewProjTransform * poly._t);
            _pInternal->_colorShader.SetMat4(_pInternal->_colorShaderUniforms[ColorShaderUniforms::uModelTrans], poly._t);
            Mat3 modelTransInv;
            bool success = poly._t.GetMat3().TransposeInverse(modelTransInv);
            assert(success);
            _pInternal->_colorShader.SetMat3(_pInternal->_colorShaderUniforms[ColorShaderUniforms::uModelInvTrans], modelTransInv);
            
        }
    }

    // Textured models
#if 0    
    {        
        Shader& shader = _pInternal->_textureShader;
        shader.Use();
        // TODO update to use new multi lights!!!
        assert(false);
        shader.SetVec3("uLightPos", light._p);
        shader.SetVec3("uAmbient", light._ambient);
        shader.SetVec3("uDiffuse", light._diffuse);
        for (int modelIx = 0; modelIx < _pInternal->_texturedModelInstances.GetCount(); ++modelIx) {
            TexturedModelInstance const* m = _pInternal->_texturedModelInstances.GetItemAtIndex(modelIx);
            Mat4 const& transMat = m->_transform;
            shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
            shader.SetMat4("uModelTrans", transMat);
            Mat3 modelTransInv;
            bool success = transMat.GetMat3().TransposeInverse(modelTransInv);
            assert(success);
            shader.SetMat3("uModelInvTrans", modelTransInv);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m->_textureId);
            glBindVertexArray(m->_mesh->_vao);
            glDrawElements(GL_TRIANGLES, /*count=*/m->_mesh->_numIndices, GL_UNSIGNED_INT, /*start_offset=*/0);
        }
    }
#endif

    // WIREFRAME
    {
        // For wireframes, we want to draw backfaces and we want to respect the
        // depth buffer, but we don't want the wireframe to actually update the
        // depth buffer!
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        
        Shader& shader = _pInternal->_wireframeShader;
        shader.Use();
        int* uniforms = _pInternal->_wireframeShaderUniforms;
        glBindVertexArray(_pInternal->_cubeWireframeMesh._vao);
        for (BoundingBoxInstance const& bb : _pInternal->_boundingBoxesToDraw) {
            shader.SetMat4(uniforms[WireframeShaderUniforms::uMvpTrans], viewProjTransform * bb._t);
            shader.SetVec4(uniforms[WireframeShaderUniforms::uColor], bb._color);
            glDrawElements(GL_TRIANGLES, /*count=*/_pInternal->_cubeWireframeMesh._numIndices, GL_UNSIGNED_INT, 0);
        }
        _pInternal->_boundingBoxesToDraw.clear();

        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
    }
    

    // TEXT RENDERING
    {
        glClear(GL_DEPTH_BUFFER_BIT);
        
        ViewportInfo const& vp = _pInternal->_g->_viewportInfo;
        Mat4 projection = Mat4::Ortho(0.f, (float)vp._width, 0.f, (float)vp._height, -1.f, 1.f);

        unsigned int fontTextureId = _pInternal->_textureIdMap.at("font");

        Shader& shader = _pInternal->_textShader;
        shader.Use();
        shader.SetMat4("uProjection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fontTextureId);
        glBindVertexArray(_pInternal->_textVao);
        Vec4 quadVertices[4];
        Vec4 cachedColor(-1.f, -1.f, -1.f, -1.f);
        for (GlyphInstance const& glyph : _pInternal->_glyphsToDraw) {
            if (glyph._colorRgba != cachedColor) {
                cachedColor = glyph._colorRgba;
                shader.SetVec4("uTextColor", cachedColor);
            }
            stbtt_aligned_quad const& q = glyph._quad;
            quadVertices[0].Set(q.x0, q.y0, q.s0, q.t0);
            quadVertices[1].Set(q.x0, q.y1, q.s0, q.t1);
            quadVertices[2].Set(q.x1, q.y0, q.s1, q.t0);
            quadVertices[3].Set(q.x1, q.y1, q.s1, q.t1);

            glBindBuffer(GL_ARRAY_BUFFER, _pInternal->_textVbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        
        _pInternal->_glyphsToDraw.clear();
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
        SetLightUniformsColorShader(*_pInternal);
        for (ColorModelInstance const* m : _pInternal->_topLayerColorModels) {
            if (!m->_visible) {
                continue;
            }
            Mat4 const& transMat = m->_transform;
            shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
            shader.SetMat4("uModelTrans", transMat);
            Mat3 modelTransInv;
            bool success = transMat.GetMat3().TransposeInverse(modelTransInv);
            assert(success);
            shader.SetMat3("uModelInvTrans", modelTransInv);

            // TEMPORARY! What we really want is to use the submeshes always unless the outer Model
            // has defined some overrides.
            if (m->_mesh->_subMeshes.size() == 0) {
                shader.SetVec4("uColor", m->_color);
                glBindVertexArray(m->_mesh->_vao);
                glDrawElements(GL_TRIANGLES, /*count=*/m->_mesh->_numIndices, GL_UNSIGNED_INT, /*start_offset=*/0);
            } else {
                for (int subMeshIx = 0; subMeshIx < m->_mesh->_subMeshes.size(); ++subMeshIx) {
                    BoundMeshPNU::SubMesh const& subMesh = m->_mesh->_subMeshes[subMeshIx];
                    shader.SetVec4("uColor", subMesh._color);
                    glBindVertexArray(m->_mesh->_vao);
                    uint64_t offset = sizeof(uint32_t) * subMesh._startIndex;
                    glDrawElements(GL_TRIANGLES, /*count=*/subMesh._numIndices, GL_UNSIGNED_INT, (void*) offset);
                }
            }
        }
    }

    //
    // transparent models
    //

    // TODO: should we cache the dists so that we don't recompute them every time we
    // examine an element?
    std::sort(_pInternal->_transparentModels.begin(), _pInternal->_transparentModels.end(), [&](ColorModelInstance const* lhs, ColorModelInstance const* rhs) {
        float lhsD2 = (lhs->_transform.GetPos() - _camera._transform.GetPos()).Length2();
        float rhsD2 = (rhs->_transform.GetPos() - _camera._transform.GetPos()).Length2();
        return lhsD2 < rhsD2;
    });
    {
        Shader& shader = _pInternal->_colorShader;
        shader.Use();
        SetLightUniformsColorShader(*_pInternal);
        for (ColorModelInstance const* m : _pInternal->_transparentModels) {
            if (!m->_visible) {
                continue;
            }
            Mat4 const& transMat = m->_transform;
            shader.SetMat4("uMvpTrans", viewProjTransform * transMat);
            shader.SetMat4("uModelTrans", transMat);
            Mat3 modelTransInv;
            bool success = transMat.GetMat3().TransposeInverse(modelTransInv);
            assert(success);
            shader.SetMat3("uModelInvTrans", modelTransInv);

            // TEMPORARY! What we really want is to use the submeshes always unless the outer Model
            // has defined some overrides.            
            if (m->_mesh->_subMeshes.size() == 0) {
                shader.SetVec4("uColor", m->_color);
                glBindVertexArray(m->_mesh->_vao);
                glDrawElements(GL_TRIANGLES, /*count=*/m->_mesh->_numIndices, GL_UNSIGNED_INT, /*start_offset=*/0);
            } else {
                for (int subMeshIx = 0; subMeshIx < m->_mesh->_subMeshes.size(); ++subMeshIx) {
                    BoundMeshPNU::SubMesh const& subMesh = m->_mesh->_subMeshes[subMeshIx];
                    shader.SetVec4("uColor", subMesh._color);
                    glBindVertexArray(m->_mesh->_vao);
                    uint64_t offset = sizeof(uint32_t) * subMesh._startIndex;
                    glDrawElements(GL_TRIANGLES, /*count=*/subMesh._numIndices, GL_UNSIGNED_INT, (void*) offset);
                }
            }
        }
    }
}

} // namespace renderer
