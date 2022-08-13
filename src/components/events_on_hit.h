#pragma once

#include "component.h"

class ScriptAction;

class EventsOnHitComponent : public Component {
public:
    // Takes entity ID of other object that did the hitting.
    typedef std::function<void(EntityId)> OnHitCallback;

    // TODO: return an ID that can be used to unregister callbacks.
    void AddOnHitCallback(OnHitCallback f);

    virtual ComponentType Type() const override { return ComponentType::EventsOnHit; }
    EventsOnHitComponent();
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    // Currently only called by Player.
    void OnHit(EntityId other) const;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    virtual void OnEditPick() override;

    void Save(ptree& pt) const override;
    void Load(ptree const& pt) override;

    void Save(serial::Ptree pt) const override;

    // Serialized
    std::vector<std::unique_ptr<ScriptAction>> _actions;
    
    std::vector<OnHitCallback> _callbacks;

    GameManager* _g = nullptr;
};