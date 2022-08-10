#pragma once

#include "component.h"

class ModelComponent;

class RecorderBalloonComponent : public Component {
public:
    virtual ~RecorderBalloonComponent() override {}
    
    virtual ComponentType Type() const override {
        return ComponentType::RecorderBalloon;
    };

    virtual void Update(float dt) override;

    // virtual void Destroy() override;
    // virtual void EditDestroy() override;

    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;   

    // If true, request that we try reconnecting the entity's components.
    // virtual bool DrawImGui() override;

    // virtual void OnEditPick() {}
    // virtual void EditModeUpdate(float dt) {}

    // virtual void Save(boost::property_tree::ptree& pt) const override;
    // virtual void Load(boost::property_tree::ptree const& pt) override;

    void OnHit(EntityId other);

    // serialized

    // non-serialized
    float _heat = 0.f;
    std::weak_ptr<ModelComponent> _model;
};