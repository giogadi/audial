#pragma once

#include <vector>
#include <string>

#include "component.h"
#include "matrix.h"

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

class SceneInternal;
class Scene {
public:
    Scene();
    virtual ~Scene();

    void AddModel(std::weak_ptr<ModelComponent const> m) { _models.push_back(m); }
    // void AddLight(std::weak_ptr<LightComponent const> l) { _lights.push_back(l); }    
    void AddCamera(std::weak_ptr<CameraComponent const> c) { _cameras.push_back(c); }

    // DON'T STORE THE POINTER!!!
    std::pair<VersionId, PointLight*> AddPointLight();
    PointLight* GetPointLight(VersionId id);
    bool RemovePointLight(VersionId id);

    void Draw(int windowWidth, int windowHeight);
    
    std::vector<std::weak_ptr<ModelComponent const>> _models;
    // std::vector<std::weak_ptr<LightComponent const>> _lights;    
    std::vector<std::weak_ptr<CameraComponent const>> _cameras;

private:
    std::unique_ptr<SceneInternal> _pInternal;
};

}  // renderer