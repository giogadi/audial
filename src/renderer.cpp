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
#include "geometry.h"

#define DRAW_WATER 0
#define DRAW_TERRAIN 1

namespace {
constexpr int kMaxLineCount = 10;
int constexpr kMaxNumPointLights = 2;

constexpr int kShadowWidth = 2 * 1024;
constexpr int kShadowHeight = 2 * 1024;

float quadVertices[] = {
       // positions   // texCoords
       -1.0f,  1.0f,  0.0f, 1.0f,
       -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
       -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
};
}

namespace renderer {

struct Lights {
    Light _dirLight;
    std::array<Light, kMaxNumPointLights> _pointLights;
};

Mat4 Camera::GetViewMatrix() const {
    Vec3 p = _transform.GetPos();
    Vec3 forward = -_transform.GetCol3(2);  // Z-axis points backward
    Vec3 up = _transform.GetCol3(1);
    return Mat4::LookAt(p, p + forward, up);
}

namespace {
bool CreateTextureFromFile(char const* filename, unsigned int& textureId, bool gammaCorrection, bool flip=true) {
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
    GLenum srcFormat;
    switch (texNumChannels) {
        case 3: format = gammaCorrection ? GL_SRGB : GL_RGB; srcFormat = GL_RGB; break;
        case 4: format = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA; srcFormat = GL_RGBA; break;
        default: assert(false); break;
    }
    glTexImage2D(
        GL_TEXTURE_2D, /*mipmapLevel=*/0, /*textureFormat=*/format, texWidth, texHeight, /*legacy=*/0,
        /*sourceFormat=*/srcFormat, /*sourceDataType=*/GL_UNSIGNED_BYTE, texData);
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

struct TextWorldInstance {
    Vec4 _colorRgba;
    Vec3 _pos;
    float _scale = 1.f;
    std::string _text;
    bool _appendToPrevious = false;
};

struct GlyphInstance {
    Vec4 _colorRgba;
    stbtt_aligned_quad _quad;
};

namespace ModelShaderUniforms {
    enum Names {
        uColor,
        uExplodeVec,
        uMvpTrans,
        uModelTrans,
        uModelInvTrans,
        uViewPos,
        uLightViewProjT,
        uDirLightDir,
        uDirLightColor,
        uDirLightAmb,
        uDirLightDif,
        uPointLight0Pos,
        uPointLight0Color,
        uPointLight0Amb,
        uPointLight0Dif,
        uPointLight0Spec,
        uPointLight1Pos,
        uPointLight1Color,
        uPointLight1Amb,
        uPointLight1Dif,
        uPointLight1Spec,
        Count
    };
    static char const* NameStrings[] = {
        "uColor",
        "uExplodeVec",
        "uMvpTrans",
        "uModelTrans",
        "uModelInvTrans",
        "uViewPos",
        "uLightViewProjT",
        "uDirLight._dir",
        "uDirLight._color",
        "uDirLight._ambient",
        "uDirLight._diffuse",
        "uPointLights[0]._pos",
        "uPointLights[0]._color",
        "uPointLights[0]._ambient",
        "uPointLights[0]._diffuse",
        "uPointLights[0]._specular",
        "uPointLights[1]._pos",
        "uPointLights[1]._color",
        "uPointLights[1]._ambient",
        "uPointLights[1]._diffuse",
        "uPointLights[1]._specular"
    };
};

#if DRAW_WATER
namespace WaterShaderUniforms {
    enum Names {
        uMvpTrans,
        uModelTrans,
        uColor,
        uDirLightDir,
        uDirLightColor,
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
        "uDirLight._color",
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
        // uModelTrans,
        // uModelInvTrans,
        uColor,
        uDirLightDir,
        // uDirLightAmb,
        uDirLightDif,
        uOffset,
        Count
    };
    static char const* NameStrings[] = {
        "uMvpTrans",
        // "uModelTrans",
        // "uModelInvTrans",
        "uColor",
        "uDirLight._dir",
        // "uDirLight._ambient",
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

namespace DepthOnlyShaderUniforms {
    enum Names {
        uViewProjT,
        uModelT,
        Count
    };
    static char const* NameStrings[] = {
        "uViewProjT", 
        "uModelT"
    };
};


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
    SceneInternal() {
        
        _topLayerModels.reserve(100);
        _transparentModels.reserve(100);
        _modelsToDraw.reserve(100);
    }
    bool Init(GameManager& g);

    bool LoadPsButtons();

    std::vector<Light> _lightsToDraw;
    std::vector<TextWorldInstance> _textToDraw;
    std::vector<GlyphInstance> _glyphsToDraw;
    std::vector<Glyph3dInstance> _glyph3dsToDraw;
    std::vector<BoundingBoxInstance> _boundingBoxesToDraw;
    std::vector<Polygon2dInstance> _polygonsToDraw;
    std::vector<LineInstance> _linesToDraw;
    std::vector<ModelInstance> _modelsToDraw;

    std::unordered_map<std::string, std::unique_ptr<BoundMeshPNU>> _meshMap;    
    std::unordered_map<std::string, unsigned int> _textureIdMap;
    std::vector<std::pair<Scene::MeshId, std::unique_ptr<BoundMeshPNU>>> _dynLoadedMeshes;
    int _nextMeshId = 0;

    BoundMeshPB _cubeWireframeMesh;

    unsigned int _whiteTextureId;   
    
    Shader _modelShader;
    int _modelShaderUniforms[ModelShaderUniforms::Count];

#if DRAW_WATER    
    Shader _waterShader;
    int _waterShaderUniforms[WaterShaderUniforms::Count];
#endif    

#if DRAW_TERRAIN    
    Shader _terrainShader;
    int _terrainShaderUniforms[TerrainShaderUniforms::Count];
#endif    
    
    Shader _textShader;
    Shader _text3dShader;

    unsigned int _textVao = 0;
    unsigned int _textVbo = 0;
    unsigned int _text3dVao = 0;
    unsigned int _text3dVbo = 0;
    std::vector<stbtt_bakedchar> _fontCharInfo;

    Shader _wireframeShader;
    int _wireframeShaderUniforms[WireframeShaderUniforms::Count];

    Shader _lineShader;
    unsigned int _lineVao = 0;
    unsigned int _lineVbo = 0;

    Shader _texturedQuadShader;
    unsigned int _texturedQuadVao = 0;
    unsigned int _texturedQuadVbo = 0;

    Shader _depthOnlyShader;
    int _depthOnlyShaderUniforms[DepthOnlyShaderUniforms::Count];
    unsigned int _depthMapFbo = 0;
    unsigned int _depthMap = 0;
    

    std::array<std::unique_ptr<BoundMeshPNU>, (int)InputManager::ControllerButton::Count> _psButtonMeshes;
    unsigned int _psButtonsTextureId;

    bool _drawTerrain = false;
    bool _enableGammaCorrection = false;

    // boring cached things
    std::vector<ModelInstance const*> _topLayerModels;
    std::vector<ModelInstance const*> _transparentModels;
    BoundMeshPNU const* _cubeMesh = nullptr;
    std::vector<float> _lineVertexData;

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

bool SceneInternal::LoadPsButtons() {
    if (!CreateTextureFromFile("data/textures/ps_buttons.png", _psButtonsTextureId, _enableGammaCorrection)) {
        return false;
    }

    std::array<float, 4 * BoundMeshPNU::kNumValuesPerVertex> vertexData;
    // top-left
    vertexData[0] = -1.f;
    vertexData[1] = 0.f;
    vertexData[2] = -1.f;
    // bottom-left
    vertexData[BoundMeshPNU::kNumValuesPerVertex*1 + 0] = -1.f;
    vertexData[BoundMeshPNU::kNumValuesPerVertex*1 + 1] = 0.f;
    vertexData[BoundMeshPNU::kNumValuesPerVertex*1 + 2] = 1.f;
    // bottom-right
    vertexData[BoundMeshPNU::kNumValuesPerVertex*2 + 0] = 1.f;
    vertexData[BoundMeshPNU::kNumValuesPerVertex*2 + 1] = 0.f;
    vertexData[BoundMeshPNU::kNumValuesPerVertex*2 + 2] = 1.f;
    //top-right
    vertexData[BoundMeshPNU::kNumValuesPerVertex*3 + 0] = 1.f;
    vertexData[BoundMeshPNU::kNumValuesPerVertex*3 + 1] = 0.f;
    vertexData[BoundMeshPNU::kNumValuesPerVertex*3 + 2] = -1.f;

    // normals
    for (int i = 0; i < 4; ++i) {
        int normalStart = (BoundMeshPNU::kNumValuesPerVertex * i) + 3;
        vertexData[normalStart] = 0.f;
        vertexData[normalStart + 1] = 1.f;
        vertexData[normalStart + 2] = 0.f;
    }

    // uses triangle fan
    uint32_t indices[] = {
        0, 1, 2, 3
    };
    
    float constexpr kIconSize = 64.f;
    float constexpr kSheetLength = kIconSize * static_cast<int>(InputManager::ControllerButton::Count);
    for (int i = 0; i < (int)InputManager::ControllerButton::Count; ++i) {
        float startU = kIconSize * i / kSheetLength;
        float endU = (kIconSize * i + kIconSize - 1.f) / kSheetLength;
        // uv's
        // 
        // top-left
        vertexData[BoundMeshPNU::kNumValuesPerVertex * 0 + 6] = startU;
        vertexData[BoundMeshPNU::kNumValuesPerVertex * 0 + 7] = 1.f;
        // bottom-left
        vertexData[BoundMeshPNU::kNumValuesPerVertex * 1 + 6] = startU;
        vertexData[BoundMeshPNU::kNumValuesPerVertex * 1 + 7] = 0.f;
        // bottom-right
        vertexData[BoundMeshPNU::kNumValuesPerVertex * 2 + 6] = endU;
        vertexData[BoundMeshPNU::kNumValuesPerVertex * 2 + 7] = 0.f;
        // top-right
        vertexData[BoundMeshPNU::kNumValuesPerVertex * 3 + 6] = endU;
        vertexData[BoundMeshPNU::kNumValuesPerVertex * 3 + 7] = 1.f;

        auto mesh = std::make_unique<BoundMeshPNU>();
        mesh->Init(vertexData.data(), 4, indices, 4);
        mesh->_useTriangleFan = true;

        _psButtonMeshes[i] = std::move(mesh);
    }

    return true;
}

bool SceneInternal::Init(GameManager& g) {
    _g = &g;

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

        mesh = std::make_unique<BoundMeshPNU>();
        success = mesh->Init("data/models/chunked_sphere.obj");
        assert(success);
        success = _meshMap.emplace("chunked_sphere", std::move(mesh)).second;
        assert(success);

        mesh = std::make_unique<BoundMeshPNU>();
        success = mesh->Init("data/models/sphere.obj");
        assert(success);
        success = _meshMap.emplace("sphere", std::move(mesh)).second;
        assert(success);

        mesh = std::make_unique<BoundMeshPNU>();
        success = mesh->Init("data/models/quad.obj");
        assert(success);
        success = _meshMap.emplace("quad", std::move(mesh)).second;
        assert(success);

        mesh = MakeWaterMesh();
        success = _meshMap.emplace("water", std::move(mesh)).second;
        assert(success);

        success = LoadPsButtons();
        assert(success);
    }

    {
        bool success = _cubeWireframeMesh.Init("data/models/cube.obj");
        if (!success) {
            return false;
        }
    }

    if (!_modelShader.Init("shaders/shader.vert", "shaders/shader.frag")) {
        return false;
    }
    for (int i = 0; i < ModelShaderUniforms::Count; ++i) {
        _modelShaderUniforms[i] = _modelShader.GetUniformLocation(ModelShaderUniforms::NameStrings[i]);
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

    if (!_text3dShader.Init("shaders/text3d.vert", "shaders/text3d.frag")) {
        return false;
    }


    if (!_wireframeShader.Init("shaders/wireframe.vert", "shaders/wireframe.frag", "shaders/wireframe.geom")) {
        return false;
    }
    for (int i = 0; i < WireframeShaderUniforms::Count; ++i) {
        _wireframeShaderUniforms[i] = _wireframeShader.GetUniformLocation(WireframeShaderUniforms::NameStrings[i]);
    }

    if (!_lineShader.Init("shaders/line.vert", "shaders/line.frag")) {
        return false;
    }

    if (!_depthOnlyShader.Init("shaders/simple_depth_shader.vert", "shaders/simple_depth_shader.frag")) {
        return false;
    }
    for (int i = 0; i < DepthOnlyShaderUniforms::Count; ++i) {
        _depthOnlyShaderUniforms[i] = _depthOnlyShader.GetUniformLocation(DepthOnlyShaderUniforms::NameStrings[i]);
    }

    if (!_texturedQuadShader.Init("shaders/quad.vert", "shaders/quad.frag")) {
        return false;
    }

    if (!CreateTextureFromFile("data/textures/white.png", _whiteTextureId, _enableGammaCorrection)) {
        return false;
    }
    _textureIdMap.emplace("white", _whiteTextureId);
 
    unsigned int woodboxTextureId = 0;
    if (!CreateTextureFromFile("data/textures/wood_container.jpg", woodboxTextureId, _enableGammaCorrection)) {
        return false;
    }
    _textureIdMap.emplace("wood_box", woodboxTextureId);

    unsigned int perlinNoiseTextureId = 0;
    if (!CreateTextureFromFile("data/textures/perlin_noise.png", perlinNoiseTextureId, /*gammaCorrection=*/false)) {
        return false;
    }
    _textureIdMap.emplace("perlin_noise", perlinNoiseTextureId);

    unsigned int perlinNoiseNormalTextureId = 0;
    if (!CreateTextureFromFile("data/textures/perlin_noise_normal.png", perlinNoiseNormalTextureId, /*gammaCorrect=*/false)) {
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

    // 3D Text stuff (2 attributes: pos (3 values) and texcoord (2 values)
    glGenVertexArrays(1, &_text3dVao);
    glGenBuffers(1, &_text3dVbo);
    glBindVertexArray(_text3dVao);
    glBindBuffer(GL_ARRAY_BUFFER, _text3dVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float))); 

    // LINE DRAWING STUFF
    {
        glGenVertexArrays(1, &_lineVao);
        glBindVertexArray(_lineVao);
        
        constexpr int kNumValuesPerVertex = 3 + 4;
        glGenBuffers(1, &_lineVbo);
        glBindBuffer(GL_ARRAY_BUFFER, _lineVbo);
        
        glBufferData(GL_ARRAY_BUFFER, kMaxLineCount * 2 * kNumValuesPerVertex * sizeof(float), NULL, GL_DYNAMIC_DRAW);

        // pos attrib
        glVertexAttribPointer(/*attributeIndex=*/0, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false, /*stride=*/kNumValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)0);
        glEnableVertexAttribArray(0);

        // color attrib
        glVertexAttribPointer(/*attributeIndex=*/1, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false, /*stride=*/kNumValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(/*attributeIndex=*/1);

        _lineVertexData.reserve(kMaxLineCount * 2 * kNumValuesPerVertex);
    }

    // SIMPLE QUAD DRAWING STUFF
    {
        glGenVertexArrays(1, &_texturedQuadVao);
        glBindVertexArray(_texturedQuadVao);
        constexpr int kNumValuesPerVertex = 2 + 2;
        glGenBuffers(1, &_texturedQuadVbo);
        glBindBuffer(GL_ARRAY_BUFFER, _texturedQuadVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * kNumValuesPerVertex * 6, quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, false, kNumValuesPerVertex*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, false, kNumValuesPerVertex*sizeof(float), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1); 
    }
    

    _cubeMesh = _meshMap["cube"].get();

    // SHADOW STUFF
    // TODO: destroy the framebuffer somewhere
    glGenFramebuffers(1, &_depthMapFbo);
    glGenTextures(1, &_depthMap);
    glBindTexture(GL_TEXTURE_2D, _depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, kShadowWidth, kShadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // float borderColor[] = { 1.f, 1.f, 1.f, 1.f };
    float borderColor[] = { 0.f, 0.f, 0.f, 1.f };
    
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


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

unsigned int Scene::GetTextureId(std::string const& textureName) const {
    auto textureIdIter = _pInternal->_textureIdMap.find(textureName);
    if (textureIdIter == _pInternal->_textureIdMap.end()) {
        return _pInternal->_whiteTextureId;
    }
    return textureIdIter->second;
}

renderer::ModelInstance* Scene::DrawTexturedMesh(BoundMeshPNU const* m, unsigned int textureId) {
    renderer::ModelInstance& model = _pInternal->_modelsToDraw.emplace_back();
    model._mesh = m;
    model._textureId = textureId;
    return &model;
}

renderer::ModelInstance* Scene::DrawMesh(MeshId id) {
    for (auto const& entry : _pInternal->_dynLoadedMeshes) {
        if (entry.first._id == id._id) {
            renderer::ModelInstance* model = &(_pInternal->_modelsToDraw.emplace_back());
            model->_mesh = entry.second.get();
            return model;
        }
    }
    return nullptr;
}

renderer::ModelInstance& Scene::DrawMesh(BoundMeshPNU const* mesh, Mat4 const& t, Vec4 const& color) {
    ModelInstance& model = _pInternal->_modelsToDraw.emplace_back();
    model._transform = t;
    model._mesh = mesh;
    model._color = color;
    model._textureId = _pInternal->_whiteTextureId;
    return model;
}

renderer::ModelInstance& Scene::DrawCube(Mat4 const& t, Vec4 const& color) {
    return DrawMesh(_pInternal->_cubeMesh, t, color);
}

void Scene::DrawBoundingBox(Mat4 const& t, Vec4 const& color) {
    _pInternal->_boundingBoxesToDraw.emplace_back();
    BoundingBoxInstance& bb = _pInternal->_boundingBoxesToDraw.back();
    bb._t = t;
    bb._color = color;
}

void Scene::DrawTextWorld(std::string text, Vec3 const& pos, float scale, Vec4 const& colorRgba, bool appendToPrevious) {
    TextWorldInstance& t = _pInternal->_textToDraw.emplace_back();
    t._text = std::move(text);
    t._pos = pos;
    t._scale = scale;
    t._colorRgba = colorRgba;
    t._appendToPrevious = appendToPrevious;
}

size_t Scene::DrawText3d(char const* text, size_t const textLength, Mat4 const& t, Vec4 const& colorRgba, BBox2d* bbox) {
    size_t const firstCharIndex = _pInternal->_glyph3dsToDraw.size();
    float currentX = 0.f;
    float currentY = 0.f;
    for (int textIx = 0; textIx < textLength; ++textIx) {
        char c = text[textIx];
        char constexpr kFirstChar = 65;  // 'A'
        int constexpr kNumChars = 58;
        int constexpr kBmpWidth = 512;
        int constexpr kBmpHeight = 128;
        int charIndex = c - kFirstChar;
        if (charIndex >= kNumChars || charIndex < 0) {
            printf("ERROR: character \'%c\' not in font!\n", c);
            assert(false); 
        }

        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(_pInternal->_fontCharInfo.data(), kBmpWidth, kBmpHeight, charIndex, &currentX, &currentY, &quad, /*opengl_fillrule=*/1);

        float constexpr kScale = 0.01875f;
        quad.x0 *= kScale;
        quad.x1 *= kScale;
        quad.y0 *= kScale;
        quad.y1 *= kScale;

        if (bbox) {
            if (textIx == 0) {
                bbox->minX = quad.x0;
                bbox->maxX = quad.x1;
                bbox->minY = quad.y0;
                bbox->maxY = quad.y1;
            } else {
                bbox->minX = std::min(bbox->minX, quad.x0);
                bbox->maxX = std::max(bbox->maxX, quad.x1);
                bbox->minY = std::min(bbox->minY, quad.y0);
                bbox->maxY = std::max(bbox->maxY, quad.y1);
            }
        }

        Glyph3dInstance& instance = _pInternal->_glyph3dsToDraw.emplace_back();
        instance.x0 = quad.x0; instance.y0 = quad.y0; instance.s0 = quad.s0; instance.t0 = quad.t0;
        instance.x1 = quad.x1; instance.y1 = quad.y1; instance.s1 = quad.s1; instance.t1 = quad.t1; 
        instance._t = t;
        instance._colorRgba = colorRgba; 
    }

    return firstCharIndex;
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

void Scene::DrawLine(Vec3 const& start, Vec3 const& end, Vec4 const& color) {
    if (_pInternal->_linesToDraw.size() == kMaxLineCount) {
        printf("WARNING: Tried to draw too many lines!\n");
        return;
    }
    LineInstance& line = _pInternal->_linesToDraw.emplace_back();
    line._start = start;
    line._end = end;
    line._color = color;
}

renderer::ModelInstance* Scene::DrawPsButton(InputManager::ControllerButton button, Mat4 const& t) {
    BoundMeshPNU const* mesh = _pInternal->_psButtonMeshes[(int)button].get();

    ModelInstance* instance = DrawTexturedMesh(mesh, _pInternal->_psButtonsTextureId);
    instance->_transform = t;
    return instance;
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
    centroid = centroid / static_cast<float>(points.size());

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
void SetLightUniformsModelShader(Lights const& lights, Vec3 const& viewPos, Mat4 const& lightViewProjT, SceneInternal& sceneInternal) {
    int const* uniforms = sceneInternal._modelShaderUniforms;
    Shader& shader = sceneInternal._modelShader;
    shader.SetVec3(uniforms[ModelShaderUniforms::uDirLightDir], lights._dirLight._dir);
    shader.SetVec3(uniforms[ModelShaderUniforms::uDirLightColor], lights._dirLight._color);
    shader.SetFloat(uniforms[ModelShaderUniforms::uDirLightAmb], lights._dirLight._ambient);
    shader.SetFloat(uniforms[ModelShaderUniforms::uDirLightDif], lights._dirLight._diffuse);
    shader.SetVec3(uniforms[ModelShaderUniforms::uViewPos], viewPos);
    shader.SetMat4(uniforms[ModelShaderUniforms::uLightViewProjT], lightViewProjT);
    for (int i = 0; i < kMaxNumPointLights; ++i) {
        int constexpr kNumUniforms = 5;
        int posLoc = uniforms[ModelShaderUniforms::uPointLight0Pos + kNumUniforms * i];
        int colorLoc = uniforms[ModelShaderUniforms::uPointLight0Color + kNumUniforms * i];
        int ambLoc = uniforms[ModelShaderUniforms::uPointLight0Amb + kNumUniforms * i];
        int difLoc = uniforms[ModelShaderUniforms::uPointLight0Dif + kNumUniforms * i];
        int specLoc = uniforms[ModelShaderUniforms::uPointLight0Spec + kNumUniforms * i];
        Light const& pl = lights._pointLights[i];
        shader.SetVec3(posLoc, pl._p);
        shader.SetVec3(colorLoc, pl._color);
        shader.SetFloat(ambLoc, pl._ambient);
        shader.SetFloat(difLoc, pl._diffuse);
        shader.SetFloat(specLoc, pl._specular);
    }
}

#if DRAW_WATER
void SetLightUniformsWaterShader(Lights const& lights, SceneInternal& sceneInternal) {
    int const* uniforms = sceneInternal._waterShaderUniforms;
    Shader& shader = sceneInternal._waterShader;
    shader.SetVec3(uniforms[WaterShaderUniforms::uDirLightDir], lights._dirLight._dir);
    shader.SetVec3(uniforms[WaterShaderUniforms::uDirLightAmb], lights._dirLight._ambient);
    shader.SetVec3(uniforms[WaterShaderUniforms::uDirLightDif], lights._dirLight._diffuse);
}
#endif

#if DRAW_TERRAIN
void SetLightUniformsTerrainShader(Lights const& lights, SceneInternal& sceneInternal) {
    int const* uniforms = sceneInternal._terrainShaderUniforms;
    Shader& shader = sceneInternal._terrainShader;
    shader.SetVec3(uniforms[TerrainShaderUniforms::uDirLightDir], lights._dirLight._dir);
    // shader.SetVec3(uniforms[TerrainShaderUniforms::uDirLightAmb], lights._dirLight._ambient);
    Vec3 difVec = lights._dirLight._color * lights._dirLight._diffuse;
    shader.SetVec3(uniforms[TerrainShaderUniforms::uDirLightDif], difVec);
}
#endif

void DrawModelDepthOnly(SceneInternal& internal, ModelInstance const& m) {
    if (!m._visible) {
        return;
    }
    Mat4 const& transMat = m._transform;
    internal._depthOnlyShader.SetMat4(internal._depthOnlyShaderUniforms[DepthOnlyShaderUniforms::uModelT], transMat);

    assert(m._mesh != nullptr);
    glBindVertexArray(m._mesh->_vao);
    if (m._mesh->_subMeshes.size() == 0) {
        GLenum mode = GL_TRIANGLES;
        if (m._mesh->_useTriangleFan) {
            mode = GL_TRIANGLE_FAN;
        }
        glDrawElements(mode, /*count=*/m._mesh->_numIndices, GL_UNSIGNED_INT, /*start_offset=*/0);
    } else {
        for (int subMeshIx = 0; subMeshIx < m._mesh->_subMeshes.size(); ++subMeshIx) {
            BoundMeshPNU::SubMesh const& subMesh = m._mesh->_subMeshes[subMeshIx];
            // if (explodeDist > 0.f) {
            //     Vec3 explodeVec = subMesh._centroid.GetNormalized() * explodeDist;
            //     internal._modelShader.SetVec3(internal._modelShaderUniforms[ModelShaderUniforms::uExplodeVec], explodeVec);
            // }
            glBindVertexArray(m._mesh->_vao);
            glDrawElements(GL_TRIANGLES, /*count=*/subMesh._numIndices, GL_UNSIGNED_INT,
                /*start_offset=*/(void*)(sizeof(uint32_t) * subMesh._startIndex));
        }
    }
} 

void DrawModelInstance(SceneInternal& internal, Mat4 const& viewProjTransform, ModelInstance const& m, float explodeDist) {
    if (!m._visible) {
        return;
    }
    Mat4 const& transMat = m._transform;
    internal._modelShader.SetMat4(internal._modelShaderUniforms[ModelShaderUniforms::uMvpTrans], viewProjTransform * transMat);
    internal._modelShader.SetMat4(internal._modelShaderUniforms[ModelShaderUniforms::uModelTrans], transMat);
    Mat3 modelTransInv;
    bool success = transMat.GetMat3().TransposeInverse(modelTransInv);
    assert(success);
    internal._modelShader.SetMat3(internal._modelShaderUniforms[ModelShaderUniforms::uModelInvTrans], modelTransInv);
    internal._modelShader.SetVec3(internal._modelShaderUniforms[ModelShaderUniforms::uExplodeVec], Vec3());

    // TEMPORARY! What we really want is to use the submeshes always unless the outer Model
    // has defined some overrides.
    assert(m._mesh != nullptr);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m._textureId);
    // glBindTexture(GL_TEXTURE_2D, internal._depthMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, internal._depthMap);
    glBindVertexArray(m._mesh->_vao);
    if (m._mesh->_subMeshes.size() == 0) {
        internal._modelShader.SetVec4(internal._modelShaderUniforms[ModelShaderUniforms::uColor], m._color);
        GLenum mode = GL_TRIANGLES;
        if (m._mesh->_useTriangleFan) {
            mode = GL_TRIANGLE_FAN;
        }
        glDrawElements(mode, /*count=*/m._mesh->_numIndices, GL_UNSIGNED_INT, /*start_offset=*/0);
    } else {
        if (!m._useMeshColor) {
            internal._modelShader.SetVec4(internal._modelShaderUniforms[ModelShaderUniforms::uColor], m._color);
        }
        for (int subMeshIx = 0; subMeshIx < m._mesh->_subMeshes.size(); ++subMeshIx) {
            BoundMeshPNU::SubMesh const& subMesh = m._mesh->_subMeshes[subMeshIx];
            if (m._useMeshColor) {
                internal._modelShader.SetVec4(internal._modelShaderUniforms[ModelShaderUniforms::uColor], subMesh._color);
            }
            if (explodeDist > 0.f) {
                Vec3 explodeVec = subMesh._centroid.GetNormalized() * explodeDist;
                internal._modelShader.SetVec3(internal._modelShaderUniforms[ModelShaderUniforms::uExplodeVec], explodeVec);
            }
            glBindVertexArray(m._mesh->_vao);
            glDrawElements(GL_TRIANGLES, /*count=*/subMesh._numIndices, GL_UNSIGNED_INT,
                /*start_offset=*/(void*)(sizeof(uint32_t) * subMesh._startIndex));
        }
    }
}
}


void Scene::Draw(int windowWidth, int windowHeight, float timeInSecs) {

    if (_pInternal->_enableGammaCorrection) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    Lights lights = {};
    int numDir = 0;
    int numPoint = 0;
    {
        for (Light const& light : _pInternal->_lightsToDraw) {
            if (light._isDirectional) {                
                lights._dirLight = light;
                ++numDir;
            } else if (numPoint < lights._pointLights.size()) {
                lights._pointLights[numPoint] = light;
                ++numPoint;
            }
        }
        _pInternal->_lightsToDraw.clear();
    }   

    Mat4 viewProjTransform = GetViewProjTransform();

    auto& transparentModels = _pInternal->_transparentModels;
    transparentModels.clear();

    _pInternal->_topLayerModels.clear();

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
        SetLightUniformsWaterShader(lights, *_pInternal);
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
    if (_pInternal->_drawTerrain) {
        Mat4 transMat;
        Vec3 cameraPos = _camera._transform.GetPos();
        Vec3 cameraPosToTerrainPos;
        if (_camera._projectionType == Camera::ProjectionType::Orthographic) {
            cameraPosToTerrainPos.Set(-10.f, -15.f, -8.f);
        } else {
            cameraPosToTerrainPos.Set(-20.f, -5.f, -8.f);
        }
        transMat.SetTranslation(cameraPos + cameraPosToTerrainPos);
        Mat3 modelTransInv;
        assert(transMat.GetMat3().TransposeInverse(modelTransInv));

        Shader& shader = _pInternal->_terrainShader;
        shader.Use();
        SetLightUniformsTerrainShader(lights, *_pInternal);  
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

    // SHADOW MAP
    Mat4 lightViewProj;
    {
        glViewport(0, 0, kShadowWidth, kShadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, _pInternal->_depthMapFbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        // glCullFace(GL_FRONT);
        Shader& shader = _pInternal->_depthOnlyShader;
        shader.Use();
        Vec3 lightDir = lights._dirLight._dir;
        // Vec3 lightPos = lightDir * (-10.f);
        Vec3 lightPos = lights._dirLight._p;
        Vec3 lightLookAt = lightPos + lightDir;
        Vec3 lightUp(0.f, 1.f, 0.f);
        if (std::abs(Vec3::Dot(lightDir, lightUp)) < 0.00001f) {
            lightUp.Set(0.f, 0.f, 1.f);
        }
        // Mat4 lightView = Mat4::LookAt(lightPos, lightLookAt, Vec3(0.f, 1.f, 0.f));  
        // Mat4 lightView = Mat4::LookAt(lightPos, lightLookAt, Vec3(0.f, 0.f, 1.f));  
        Mat4 lightView = Mat4::LookAt(lightPos, lightLookAt, lightUp);  


        float zn = lights._dirLight._zn;
        float zf = lights._dirLight._zf;
        float ar = (float)kShadowWidth / (float)kShadowHeight;
        float width = lights._dirLight._width;
        Mat4 lightProj = Mat4::Ortho(width, ar, zn, zf);
        lightViewProj = lightProj * lightView;

        _pInternal->_depthOnlyShader.SetMat4(_pInternal->_depthOnlyShaderUniforms[DepthOnlyShaderUniforms::uViewProjT], lightViewProj);
        for (ModelInstance const& m : _pInternal->_modelsToDraw) {
            if (m._topLayer || m._color._w < 1.f) {
                continue;
            }         
            DrawModelDepthOnly(*_pInternal, m);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ViewportInfo const& viewport = _pInternal->_g->_viewportInfo;
        SetViewport(viewport); 
        glCullFace(GL_BACK);
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    // MAIN SCENE DRAW

    {
        Shader& shader = _pInternal->_modelShader;
        shader.Use();        
        // TODO: NEED TO DO THIS BETTER!!!!
        shader.SetInt("uMyTexture", 0);
        shader.SetInt("uShadowMap", 1);
        SetLightUniformsModelShader(lights, _camera._transform.GetPos(), lightViewProj, *_pInternal);
        for (ModelInstance const& m : _pInternal->_modelsToDraw) {
            if (m._topLayer) {
                _pInternal->_topLayerModels.push_back(&m);
                continue;
            } else if (m._color._w < 1.f) {
                transparentModels.push_back(&m);
                continue;
            }
            DrawModelInstance(*_pInternal, viewProjTransform, m, m._explodeDist);
        }
    }

    {
        // DEBUG RENDERING FOR DEPTH MAP

        static bool sDrawDepthMap = false;
        InputManager& input = *_pInternal->_g->_inputManager;
        if (input.IsKeyPressedThisFrame(InputManager::Key::D) && input.IsShiftPressed()) {
            sDrawDepthMap = !sDrawDepthMap;
        }
        if (sDrawDepthMap) {
        
            glViewport(0, 0, kShadowWidth, kShadowHeight);
            Shader& shader = _pInternal->_texturedQuadShader;
            shader.Use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _pInternal->_depthMap);
            glDisable(GL_DEPTH_TEST);

            glBindVertexArray(_pInternal->_texturedQuadVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);

            ViewportInfo const& viewport = _pInternal->_g->_viewportInfo;
            SetViewport(viewport);

        }
        
    }

    // Polygons (immediate API)
    //
    // TODO: not sure this works yet after the great model unification
    {
        Shader& shader = _pInternal->_modelShader;
        shader.Use();
        SetLightUniformsModelShader(lights, _camera._transform.GetPos(), lightViewProj, *_pInternal);
        for (Polygon2dInstance const& poly : _pInternal->_polygonsToDraw) {
            _pInternal->_modelShader.SetMat4(_pInternal->_modelShaderUniforms[ModelShaderUniforms::uMvpTrans], viewProjTransform * poly._t);
            _pInternal->_modelShader.SetMat4(_pInternal->_modelShaderUniforms[ModelShaderUniforms::uModelTrans], poly._t);
            Mat3 modelTransInv;
            bool success = poly._t.GetMat3().TransposeInverse(modelTransInv);
            assert(success);
            _pInternal->_modelShader.SetMat3(_pInternal->_modelShaderUniforms[ModelShaderUniforms::uModelInvTrans], modelTransInv);
            
        }
    }

    // 3D TEXT
    {
        unsigned int fontTextureId = _pInternal->_textureIdMap.at("font");
        
        Shader& shader = _pInternal->_text3dShader;
        shader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fontTextureId);
        glBindVertexArray(_pInternal->_text3dVao);

        for (Glyph3dInstance const& glyph : _pInternal->_glyph3dsToDraw) {
            shader.SetMat4("uMvpTrans", viewProjTransform * glyph._t);
            shader.SetVec4("uColor", glyph._colorRgba);
            float vertexData[5 * 4];

            int dataIx = 0;
            vertexData[dataIx++] = glyph.x0;
            vertexData[dataIx++] = 0.f;
            vertexData[dataIx++] = glyph.y0;
            vertexData[dataIx++] = glyph.s0;
            vertexData[dataIx++] = glyph.t0;

            vertexData[dataIx++] = glyph.x0;
            vertexData[dataIx++] = 0.f;
            vertexData[dataIx++] = glyph.y1;
            vertexData[dataIx++] = glyph.s0;
            vertexData[dataIx++] = glyph.t1;

            vertexData[dataIx++] = glyph.x1;
            vertexData[dataIx++] = 0.f;
            vertexData[dataIx++] = glyph.y0;
            vertexData[dataIx++] = glyph.s1;
            vertexData[dataIx++] = glyph.t0;

            vertexData[dataIx++] = glyph.x1;
            vertexData[dataIx++] = 0.f;
            vertexData[dataIx++] = glyph.y1;
            vertexData[dataIx++] = glyph.s1;
            vertexData[dataIx++] = glyph.t1;

            glBindBuffer(GL_ARRAY_BUFFER, _pInternal->_text3dVbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertexData), vertexData);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        }
        _pInternal->_glyph3dsToDraw.clear();

    }


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

    //
    // transparent models
    //
    // glClear(GL_DEPTH_BUFFER_BIT);  // TODO do we need this?

    // Sort models farthest to nearest
    // TODO: should we cache the dists so that we don't recompute them every time we
    // examine an element?
    std::sort(transparentModels.begin(), transparentModels.end(), [&](ModelInstance const* lhs, ModelInstance const* rhs) {
        // Sort by Y specifically
        Vec3 lhsP = lhs->_transform.GetPos();
        Vec3 rhsP = rhs->_transform.GetPos();
        return lhsP._y < rhsP._y;
        
        // float lhsD2 = (lhs->_transform.GetPos() - _camera._transform.GetPos()).Length2();
        // float rhsD2 = (rhs->_transform.GetPos() - _camera._transform.GetPos()).Length2();
        // return lhsD2 > rhsD2;
        });
    {
        Shader& shader = _pInternal->_modelShader;
        shader.Use();
        SetLightUniformsModelShader(lights, _camera._transform.GetPos(), lightViewProj, *_pInternal);
        for (ModelInstance const* m : transparentModels) {
            DrawModelInstance(*_pInternal, viewProjTransform, *m, m->_explodeDist);
        }
    }

    // TODO: maybe we should move all glClear()'s into renderer.cpp and save
    // game.cpp from including any GL code?
    glClear(GL_DEPTH_BUFFER_BIT);

    //
    // TEXT RENDERING
    //

    // First go through our world text strings and convert them into glyphs to draw
    {
        ViewportInfo const& vp = _pInternal->_g->_viewportInfo;
        float screenX = 0.f, screenY = 0.f;
        for (TextWorldInstance const& text : _pInternal->_textToDraw) {
            if (!text._appendToPrevious) {
                geometry::ProjectWorldPointToScreenSpace(text._pos, viewProjTransform, vp._width, vp._height, screenX, screenY);
            }

            /*screenX = std::round(screenX);
            screenY = std::round(screenY);*/
            DrawText(text._text, screenX, screenY, text._scale, text._colorRgba);
        }
        _pInternal->_textToDraw.clear();
    }

    {
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

        
    // top-layer models
    {
        Shader& shader = _pInternal->_modelShader;
        shader.Use();
        SetLightUniformsModelShader(lights, _camera._transform.GetPos(), lightViewProj, *_pInternal);
        for (ModelInstance const* m : _pInternal->_topLayerModels) {
            DrawModelInstance(*_pInternal, viewProjTransform, *m, m->_explodeDist);
        }        
    }

    // Lines
    if (!_pInternal->_linesToDraw.empty()) {
        auto& lineVertexData = _pInternal->_lineVertexData;
        lineVertexData.clear();
        for (LineInstance const& line : _pInternal->_linesToDraw) {
            Vec4 start(line._start._x, line._start._y, line._start._z, 1.f);
            start = viewProjTransform * start;
            start /= start._w;
            Vec4 end(line._end._x, line._end._y, line._end._z, 1.f);
            end = viewProjTransform * end;
            end /= end._w;
            
            lineVertexData.push_back(start._x);
            lineVertexData.push_back(start._y);
            lineVertexData.push_back(start._z);
            lineVertexData.push_back(line._color._x);
            lineVertexData.push_back(line._color._y);
            lineVertexData.push_back(line._color._z);
            lineVertexData.push_back(line._color._w);

            lineVertexData.push_back(end._x);
            lineVertexData.push_back(end._y);
            lineVertexData.push_back(end._z);
            lineVertexData.push_back(line._color._x);
            lineVertexData.push_back(line._color._y);
            lineVertexData.push_back(line._color._z);
            lineVertexData.push_back(line._color._w);
        }

        Shader& shader = _pInternal->_lineShader;
        shader.Use();
        glBindVertexArray(_pInternal->_lineVao);

        glBindBuffer(GL_ARRAY_BUFFER, _pInternal->_lineVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertexData.size() * sizeof(float), lineVertexData.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_LINES, 0, 2 * _pInternal->_linesToDraw.size());
        
        _pInternal->_linesToDraw.clear();
    }

    _pInternal->_modelsToDraw.clear();

    if (_pInternal->_enableGammaCorrection) {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }
}

renderer::Light* Scene::DrawLight() {
    return &_pInternal->_lightsToDraw.emplace_back();
}

void Scene::SetDrawTerrain(bool enable) {
    _pInternal->_drawTerrain = enable;
}

Glyph3dInstance& Scene::GetText3d(size_t id) {
    return _pInternal->_glyph3dsToDraw.at(id);
}

void Scene::SetEnableGammaCorrection(bool enable) {
    _pInternal->_enableGammaCorrection = enable;
}

bool Scene::IsGammaCorrectionEnabled() const {
    return _pInternal->_enableGammaCorrection;
}

void Scene::SetViewport(ViewportInfo const& viewport) {
    glViewport(viewport._offsetX, viewport._offsetY, viewport._width, viewport._height);
    // glViewport(0, 0, viewport._width, viewport._height);
}

} // namespace renderer
