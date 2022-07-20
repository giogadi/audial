#pragma once

#include "matrix.h"
#include "constants.h"
#include "version_id.h"

class BoundMesh;

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
    void Set(Mat4 const& t, float fovyRad, float zNear, float zFar) {
        _transform = t;
        _fovyRad = fovyRad;
        _zNear = zNear;
        _zFar = zFar;
    }

    Mat4 GetViewMatrix() const;

    Mat4 _transform;
    float _fovyRad = 45.f * kPi / 180.f;
    float _zNear = 0.1f;
    float _zFar = 100.f;
};

class ModelInstance {
public:
    void Set(Mat4 const& t, BoundMesh const* mesh, Vec4 const& color) {
        _transform = t;
        _mesh = mesh;
        _color = color;
    }

    Mat4 _transform;
    BoundMesh const* _mesh = nullptr;
    Vec4 _color;
};

class SceneInternal;
class Scene {
public:
    Scene();
    virtual ~Scene();

    // void AddModel(std::weak_ptr<ModelComponent const> m) { _models.push_back(m); }  

    // DON'T STORE THE POINTERS!!!
    std::pair<VersionId, PointLight*> AddPointLight();
    PointLight* GetPointLight(VersionId id);
    bool RemovePointLight(VersionId id);

    std::pair<VersionId, ModelInstance*> AddModelInstance();
    ModelInstance* GetModelInstance(VersionId id);
    bool RemoveModelInstance(VersionId id);

    void Draw(int windowWidth, int windowHeight);

    Camera _camera;
private:
    std::unique_ptr<SceneInternal> _pInternal;
};

}  // renderer