#pragma once

#include "component.h"

class VelocityComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Velocity; }
    VelocityComponent()
        :_linear(0.f,0.f,0.f) {}

    virtual void Update(float dt) override {
        std::shared_ptr<TransformComponent> transform = _transform.lock();
        assert(!transform->HasParent());
        transform->SetWorldPos(transform->GetWorldPos() + dt * _linear);
        Mat3 rot = transform->GetWorldRot();
        rot = Mat3::FromAxisAngle(Vec3(0.f, 1.f, 0.f), dt * _angularY) * rot;
        transform->SetWorldRot(rot);
    }

    virtual void Destroy() override {}

    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override {
        _transform = e.FindComponentOfType<TransformComponent>();
        return !_transform.expired();
    }

    virtual void Save(serial::Ptree pt) const override {
        serial::SaveInNewChildOf(pt, "linear", _linear);        
        pt.PutFloat("angularY", _angularY);
    }
    virtual void Load(serial::Ptree pt) override {
        _linear.Load(pt.GetChild("linear"));
        _angularY = pt.GetFloat("angularY");
    }

    std::weak_ptr<TransformComponent> _transform;
    Vec3 _linear;
    float _angularY = 0.0f;  // rad / s

};