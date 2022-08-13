#pragma once

#include "component.h"
#include "matrix.h"

class BoundMeshPNU;
namespace renderer {
    class Scene;
}
class TransformComponent;

class ModelComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Model; }
    ModelComponent() : _color(1.f, 1.f, 1.f, 1.f) {}
    virtual ~ModelComponent() override {}
    virtual void Destroy() override;
    virtual void EditDestroy() override;
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    virtual void Update(float dt) override;
    virtual void EditModeUpdate(float dt) override;
    virtual bool DrawImGui() override;
    virtual void Save(serial::Ptree pt) const override;
    virtual void Load(serial::Ptree pt) override;

    // serialized
    std::string _meshId;
    Vec4 _color;

    std::weak_ptr<TransformComponent const> _transform;
    renderer::Scene* _scene = nullptr;
    BoundMeshPNU const* _mesh = nullptr;
};