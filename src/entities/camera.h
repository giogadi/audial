#pragma once

#include <optional>

#include "new_entity.h"

namespace renderer {
    class Camera;
}

struct CameraEntity : ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::Camera; }
    static ne::EntityType StaticType() { return ne::EntityType::Camera; }
    
    // serialized
    bool _ortho = false;
    std::string _followEntityName;
    float _initialTrackingFactor = 0.05f;
    float _fovyDeg = 45.f;
    float _zNear = 0.1f;
    float _zFar = 100.f;
    float _width = 20.f;

    // non-serialized
    float _trackingFactor = 0.05f;
    ne::EntityId _followEntityId;
    Vec3 _desiredTargetToCameraOffset;
    std::optional<float> _minX;
    std::optional<float> _minZ;
    std::optional<float> _maxX;
    std::optional<float> _maxZ;
    float _constraintBlend = 0.f;

    void ResetConstraintBlend() { _constraintBlend = 0.f; };

    // Immediately sets camera position to this position (plus target offsets)
    void JumpToPosition(Vec3 const& p);

    virtual void InitDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override;
protected:
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    renderer::Camera* _camera = nullptr;
};
