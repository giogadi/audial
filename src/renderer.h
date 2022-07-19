#pragma once

#include <vector>
#include <string>

#include "component.h"
#include "matrix.h"
#include "constants.h"

class ModelComponent;
class LightComponent;
class CameraComponent;
class SceneManagerInternal;

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

class SceneInternal;
class Scene {
public:
    Scene();
    virtual ~Scene();

    void AddModel(std::weak_ptr<ModelComponent const> m) { _models.push_back(m); }  
    // void AddCamera(std::weak_ptr<CameraComponent const> c) { _cameras.push_back(c); }

    // DON'T STORE THE POINTER!!!
    std::pair<VersionId, PointLight*> AddPointLight();
    PointLight* GetPointLight(VersionId id);
    bool RemovePointLight(VersionId id);

    void Draw(int windowWidth, int windowHeight);
    
    std::vector<std::weak_ptr<ModelComponent const>> _models;
    // std::vector<std::weak_ptr<CameraComponent const>> _cameras;

    Camera _camera;
private:
    std::unique_ptr<SceneInternal> _pInternal;
};

}  // renderer