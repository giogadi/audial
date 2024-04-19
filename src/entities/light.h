#pragma once

#include "new_entity.h"
#include "version_id.h"

struct LightEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::Light; }
    static ne::EntityType StaticType() { return ne::EntityType::Light; }
    
    // serialized
    Vec3 _color;
    float _ambient = 0.f;
    float _diffuse = 0.f;
    float _specular = 0.f;
    float _zn = 1.f;
    float _zf = 20.f;
    float _width = 20.f;

    // If true, light shines along +z. if false, it's a point light.
    bool _isDirectional = false;

    VersionId _lightId;
    virtual void InitDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override;
    virtual void Destroy(GameManager& g) override;

    virtual void DebugPrint() override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};
