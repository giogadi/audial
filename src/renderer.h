#pragma once

#include <vector>
#include <string>

#include "component.h"

class SceneManager;
class BoundMesh;
class InputManager;
class GameManager;

class ModelComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Model; }
    ModelComponent() {}
    virtual ~ModelComponent() {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;

    std::string _modelId;
    std::weak_ptr<TransformComponent const> _transform;
    BoundMesh const* _mesh = nullptr;
    SceneManager* _mgr = nullptr;
};

class LightComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Light; }
    LightComponent()
        : _ambient(0.1f,0.1f,0.1f)
        , _diffuse(1.f,1.f,1.f)
        {}
    virtual ~LightComponent() {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;
    virtual void DrawImGui() override;

    std::weak_ptr<TransformComponent const> _transform;
    Vec3 _ambient;
    Vec3 _diffuse;
    SceneManager* _mgr = nullptr;
};

class CameraComponent : public Component {
public:  
    virtual ComponentType Type() const override { return ComponentType::Camera; }
    CameraComponent() {}
    virtual bool ConnectComponents(Entity& e, GameManager& g) override;

    virtual ~CameraComponent() {}

    Mat4 GetViewMatrix() const;

    // TODO: make movement relative to camera viewpoint
    // TODO: break movement out into its own component
    virtual void Update(float const dt) override;

    std::weak_ptr<TransformComponent> _transform;
    InputManager const* _input = nullptr;
    SceneManager* _mgr = nullptr;
};

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