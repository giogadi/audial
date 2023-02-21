#pragma once

#include <string_view>
#include <memory>

#include "matrix.h"
#include "constants.h"
#include "version_id.h"

struct GameManager;

class BoundMeshPNU;

namespace renderer {

class PointLight {
public:
    void Set(Vec3 const& p, Vec3 const& ambient, Vec3 const& diffuse) {
        _p = p;
        _ambient = ambient;
        _diffuse = diffuse;
    }

    Vec3 _p;
    Vec3 _ambient;
    Vec3 _diffuse;    
};

class Camera {
public:
    enum class ProjectionType { Perspective, Orthographic };    

    void Set(Mat4 const& t, float fovyRad, float zNear, float zFar) {
        _transform = t;
        _fovyRad = fovyRad;
        _zNear = zNear;
        _zFar = zFar;
    }

    Mat4 GetViewMatrix() const;

    Mat4 _transform;
    ProjectionType _projectionType = ProjectionType::Perspective;

    // Only used in perspective
    float _fovyRad = 45.f * kPi / 180.f;

    // Only used in ortho
    float _width = 20.f;

    float _zNear = 0.1f;
    float _zFar = 100.f;
};

class ColorModelInstance {
public:
    void Set(Mat4 const& t, BoundMeshPNU const* mesh) {
        _transform = t;
        _mesh = mesh;
        _useMeshColor = true;
    }
    void Set(Mat4 const& t, BoundMeshPNU const* mesh, Vec4 const& color) {
        _transform = t;
        _mesh = mesh;
        _color = color;
        _useMeshColor = false;
    }

    Mat4 _transform;
    BoundMeshPNU const* _mesh = nullptr;
    bool _useMeshColor = false;
    Vec4 _color;
    bool _visible = true;
    bool _topLayer = false;
};

class TexturedModelInstance {
public:
    void Set(Mat4 const& t, BoundMeshPNU const* mesh, unsigned int textureId) {
        _transform = t;
        _mesh = mesh;
        _textureId = textureId;
    }

    Mat4 _transform;
    BoundMeshPNU const* _mesh = nullptr;
    unsigned int _textureId = 0;
};

class SceneInternal;
class Scene {
public:
    Scene();    
    ~Scene();

    bool Init(GameManager& g);

    // DON'T STORE THE POINTERS!!!
    std::pair<VersionId, PointLight*> AddPointLight();
    PointLight* GetPointLight(VersionId id);
    bool RemovePointLight(VersionId id);

    std::pair<VersionId, ColorModelInstance*> AddColorModelInstance();
    ColorModelInstance* GetColorModelInstance(VersionId id);
    bool RemoveColorModelInstance(VersionId id);

    std::pair<VersionId, TexturedModelInstance*> AddTexturedModelInstance();
    TexturedModelInstance* GetTexturedModelInstance(VersionId id);
    bool RemoveTexturedModelInstance(VersionId id);

    void Draw(int windowWidth, int windowHeight, float timeInSecs);

    BoundMeshPNU const* GetMesh(std::string const& meshName) const;
    // TODO: inline or remove some of the call depth here
    renderer::ColorModelInstance& DrawMesh(BoundMeshPNU const* m, Mat4 const& t, Vec4 const& color);
    renderer::ColorModelInstance& DrawMesh();
    renderer::ColorModelInstance& DrawCube(Mat4 const& t, Vec4 const& color);

    void DrawText(std::string_view str, float& screenX, float& screenY, float scale=1.f, Vec4 const& colorRgba = Vec4(1.f, 1.f, 1.f, 1.f));

    Camera _camera;

    Mat4 GetViewProjTransform() const;
private:
    std::unique_ptr<SceneInternal> _pInternal;
};

}  // renderer
