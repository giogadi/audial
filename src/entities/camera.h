#pragma once

#include "new_entity.h"

namespace renderer {
    class Camera;
}

struct CameraEntity : ne::Entity {
    bool _ortho = false;

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
protected:
    // virtual void SaveDerived(serial::Ptree pt) const {};
    // virtual void LoadDerived(serial::Ptree pt) {};
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    renderer::Camera* _camera = nullptr;
};