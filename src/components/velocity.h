#pragma once

#include "component.h"

class VelocityComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Velocity; }
    VelocityComponent()
        :_linear(0.f,0.f,0.f) {}

    virtual void Update(float dt) override {
        std::shared_ptr<TransformComponent> transform = _transform.lock();
        transform->SetPos(transform->GetPos() + dt * _linear);
        Mat3 rot = transform->GetRot();
        rot = Mat3::FromAxisAngle(Vec3(0.f, 1.f, 0.f), dt * _angularY) * rot;
        transform->SetRot(rot);
    }

    virtual void Destroy() override {}

    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override {
        _transform = e.FindComponentOfType<TransformComponent>();
        return !_transform.expired();
    }

    virtual void Save(ptree& pt) const override {
        _linear.Save(pt.add_child("linear", ptree()));
        pt.put<float>("angularY", _angularY);
    }
    virtual void Load(ptree const& pt) override {
        _linear.Load(pt.get_child("linear"));
        _angularY = pt.get<float>("angularY");
    }

    std::weak_ptr<TransformComponent> _transform;
    Vec3 _linear;
    float _angularY = 0.0f;  // rad / s

};