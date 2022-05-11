#include "component.h"

ModelComponent::ModelComponent(TransformComponent const* trans, BoundMesh const* mesh, SceneManager* sceneMgr)
        : _transform(trans)
        , _mesh(mesh)
        , _mgr(sceneMgr) {
    _mgr->AddModel(this);
}

void ModelComponent::Destroy() {
    _mgr->RemoveModel(this);
}

LightComponent::LightComponent(TransformComponent const* t, glm::vec3 const& ambient, glm::vec3 const& diffuse, SceneManager* sceneMgr)
        : _transform(t) 
        , _ambient(ambient)
        , _diffuse(diffuse)
        , _mgr(sceneMgr) {
    _mgr->AddLight(this);
}

void LightComponent::Destroy() {
    _mgr->RemoveLight(this);
}

CameraComponent::CameraComponent(TransformComponent* t, InputManager const* input, SceneManager* sceneMgr)
        : _transform(t)
        , _input(input)
        , _mgr(sceneMgr) {
    _mgr->AddCamera(this);
}

void CameraComponent::Destroy() {
    _mgr->RemoveCamera(this);
}