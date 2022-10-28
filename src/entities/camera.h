#pragma once

#include "new_entity.h"

namespace renderer {
    class Camera;
}

struct CameraEntity : ne::Entity {
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
protected:
    // virtual void SaveDerived(serial::Ptree pt) const {};
    // virtual void LoadDerived(serial::Ptree pt) {};
    virtual void ImGuiDerived() override;

    renderer::Camera* _camera = nullptr;
};