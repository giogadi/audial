#pragma once

#include "new_entity.h"
#include "version_id.h"

struct LightEntity : public ne::Entity {
    // serialized
    Vec3 _ambient;
    Vec3 _diffuse;

    VersionId _lightId;
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void Destroy(GameManager& g) override;

    virtual void DebugPrint() override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void ImGuiDerived() override;
};