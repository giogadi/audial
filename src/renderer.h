#pragma once

#include <string_view>
#include <memory>
#include <vector>

#include "matrix.h"
#include "constants.h"
#include "version_id.h"
#include "input_manager.h"

struct GameManager;

class BoundMeshPNU;

namespace renderer {

struct Light {
    void Set(Vec3 const& p, Vec3 const& ambient, Vec3 const& diffuse) {
        _p = p;
        _ambient = ambient;
        _diffuse = diffuse;
    }
    bool _isDirectional = false;
    Vec3 _p;  // if directional, this is the direction; otherwise, it's the position.
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

class ModelInstance {
public:
    void Set(Mat4 const& t, BoundMeshPNU const* mesh, unsigned int textureId) {
        _transform = t;
        _mesh = mesh;
        _textureId = textureId;
    }

    Mat4 _transform;
    BoundMeshPNU const* _mesh = nullptr;
    unsigned int _textureId = 0;
    bool _visible = true;
    bool _topLayer = false;
    Vec4 _color = Vec4(1.f, 1.f, 1.f, 1.f);  // multiplied with the texture
    bool _useMeshColor = false;
    float _explodeDist = 0.f;
};

struct LineInstance {
    Vec3 _start;
    Vec3 _end;
    Vec4 _color;
};

class SceneInternal;
class Scene {
public:
    Scene();    
    ~Scene();

    bool Init(GameManager& g);

    // Must be called every frame to continue lighting.
    // Don't store the pointer!
    Light* DrawLight();
    

    // Returns ID to be used for drawing and deleting
    struct MeshId {
        int _id = -1;
    };
    MeshId LoadPolygon2d(std::vector<Vec3> const& points);
    // bool UnloadPolygon2d(MeshId id);

    void Draw(int windowWidth, int windowHeight, float timeInSecs);

    BoundMeshPNU const* GetMesh(std::string const& meshName) const;
    // TODO: inline or remove some of the call depth here
    ModelInstance& DrawMesh(BoundMeshPNU const* m, Mat4 const& t, Vec4 const& color);
    ModelInstance& DrawCube(Mat4 const& t, Vec4 const& color);

    ModelInstance* DrawMesh(MeshId id);

    ModelInstance* DrawTexturedMesh(BoundMeshPNU const* m, unsigned int textureId);

    void DrawBoundingBox(Mat4 const& t, Vec4 const& color);

    void DrawText(std::string_view str, float& screenX, float& screenY, float scale=1.f, Vec4 const& colorRgba = Vec4(1.f, 1.f, 1.f, 1.f));
    void DrawTextWorld(std::string text, Vec3 const& pos, float scale = 1.f, Vec4 const& colorRgba = Vec4(1.f, 1.f, 1.f, 1.f));

    void DrawLine(Vec3 const& start, Vec3 const& end, Vec4 const& color);

    ModelInstance* DrawPsButton(InputManager::ControllerButton button, Mat4 const& t);

    Camera _camera;

    Mat4 GetViewProjTransform() const;
private:
    std::unique_ptr<SceneInternal> _pInternal;
};

}  // renderer
