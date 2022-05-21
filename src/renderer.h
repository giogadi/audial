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
    virtual ComponentType Type() override { return ComponentType::Model; }
    ModelComponent() {}
    virtual ~ModelComponent() {}
    virtual void Destroy() override;
    virtual void ConnectComponents(Entity& e, GameManager& g) override;

    std::string _modelId;
    TransformComponent const* _transform = nullptr;
    BoundMesh const* _mesh = nullptr;
    SceneManager* _mgr = nullptr;
};

class LightComponent : public Component {
public:
    virtual ComponentType Type() override { return ComponentType::Light; }
    LightComponent()
        : _ambient(0.1f,0.1f,0.1f)
        , _diffuse(1.f,1.f,1.f)
        {}
    virtual ~LightComponent() {}
    virtual void Destroy() override;
    virtual void ConnectComponents(Entity& e, GameManager& g) override;
    virtual void DrawImGui() override;

    TransformComponent const* _transform = nullptr;
    Vec3 _ambient;
    Vec3 _diffuse;
    SceneManager* _mgr = nullptr;
};

class CameraComponent : public Component {
public:  
    virtual ComponentType Type() override { return ComponentType::Camera; }
    CameraComponent() {}
    virtual void ConnectComponents(Entity& e, GameManager& g) override;

    virtual ~CameraComponent() {}
    virtual void Destroy() override;

    Mat4 GetViewMatrix() const;

    // TODO: make movement relative to camera viewpoint
    // TODO: break movement out into its own component
    virtual void Update(float const dt) override;

    TransformComponent* _transform = nullptr;
    InputManager const* _input = nullptr;
    SceneManager* _mgr = nullptr;
};

class SceneManager {
public:    
    void AddModel(ModelComponent const* m) { _models.push_back(m); }
    void AddLight(LightComponent const* l) { _lights.push_back(l); }
    void AddCamera(CameraComponent const* c) { _cameras.push_back(c); }

    void RemoveModel(ModelComponent const* m);
    void RemoveLight(LightComponent const* l);
    void RemoveCamera(CameraComponent const* c);

    void Draw(int windowWidth, int windowHeight);

    std::vector<ModelComponent const*> _models;
    std::vector<LightComponent const*> _lights;
    std::vector<CameraComponent const*> _cameras;
};