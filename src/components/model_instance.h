#pragma once

#include "component.h"
#include "matrix.h"

class BoundMesh;
class SceneManager;
class TransformComponent;

class ModelComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Model; }
    ModelComponent() : _color(1.f, 1.f, 1.f, 1.f) {}
    virtual ~ModelComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    virtual bool DrawImGui() override;
    virtual void Save(ptree& pt) const override;
    virtual void Load(ptree const& pt) override;

    // serialized
    std::string _modelId;
    Vec4 _color;

    std::weak_ptr<TransformComponent const> _transform;
    BoundMesh const* _mesh = nullptr;
    SceneManager* _mgr = nullptr;
};