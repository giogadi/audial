#pragma once

#include <vector>
#include <string>

#include "component.h"
#include "matrix.h"

class ModelComponent;
class LightComponent;
class CameraComponent;

class SceneManager {
public:
    void AddModel(std::weak_ptr<ModelComponent const> m) { _models.push_back(m); }
    void AddLight(std::weak_ptr<LightComponent const> l) { _lights.push_back(l); }
    void AddCamera(std::weak_ptr<CameraComponent const> c) { _cameras.push_back(c); }

    void Draw(int windowWidth, int windowHeight);

    std::vector<std::weak_ptr<ModelComponent const>> _models;
    std::vector<std::weak_ptr<LightComponent const>> _lights;
    std::vector<std::weak_ptr<CameraComponent const>> _cameras;
};