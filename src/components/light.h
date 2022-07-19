#pragma once

#include "component.h"
#include "matrix.h"

namespace renderer {
    class Scene;
}
class TransformComponent;
class SceneManager;

class LightComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Light; }
    LightComponent()
        : _ambient(0.1f,0.1f,0.1f)
        , _diffuse(1.f,1.f,1.f)
        {}
    virtual ~LightComponent() {}
    virtual void Update(float dt) override;
    virtual void EditModeUpdate(float dt) override;
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    virtual void Destroy() override;
    virtual bool DrawImGui() override;
    virtual void Save(ptree& pt) const override;
    virtual void Load(ptree const& pt) override;

    std::weak_ptr<TransformComponent const> _transform;
    Vec3 _ambient;
    Vec3 _diffuse;
    VersionId _lightId;
    renderer::Scene* _scene = nullptr;
};