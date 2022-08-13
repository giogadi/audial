#pragma once

#include "component.h"

class RigidBodyComponent;
class TransformComponent;

class HitCounterComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::HitCounter; }
    HitCounterComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    static void OnHit(
        std::weak_ptr<HitCounterComponent> beepComp, std::weak_ptr<RigidBodyComponent> other);

    virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    void Save(ptree& pt) const override {
        pt.put("hits_remaining", _hitsRemaining);
    }
    void Save(serial::Ptree pt) const override {
        pt.PutInt("hits_remaining", _hitsRemaining);
    }
    void Load(ptree const& pt) override {
        _hitsRemaining = pt.get<int>("hits_remaining");
    }
    void Load(serial::Ptree pt) override {
        _hitsRemaining = pt.GetInt("hits_remaining");
    }

    // Serialize
    int _hitsRemaining = 0;

    std::weak_ptr<TransformComponent> _t;
    std::weak_ptr<RigidBodyComponent> _rb;
    bool _wasHit = false;
};