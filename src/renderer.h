#pragma once

#include <vector>

#include "component.h"

class SceneManager;
class BoundMesh;
class InputManager;

class ModelComponent : public Component {
public:
    ModelComponent(TransformComponent const* trans, BoundMesh const* mesh, SceneManager* sceneMgr);
    virtual ~ModelComponent() {}
    virtual void Destroy() override;

    TransformComponent const* _transform = nullptr;
    BoundMesh const* _mesh = nullptr;
    SceneManager* _mgr = nullptr;
};

class LightComponent : public Component {
public:
    LightComponent(TransformComponent const* t, Vec3 const& ambient, Vec3 const& diffuse, SceneManager* sceneMgr);
    virtual ~LightComponent() {}
    virtual void Destroy() override;

    TransformComponent const* _transform;
    Vec3 _ambient;
    Vec3 _diffuse;
    SceneManager* _mgr;
};

class CameraComponent : public Component {
public:  
    CameraComponent(TransformComponent* t, InputManager const* input, SceneManager* sceneMgr);

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