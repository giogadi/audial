#pragma once

#include "component.h"
#include "matrix.h"

class TransformComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Transform; }
    TransformComponent()
        : _localTransform(Mat4::Identity()) {}

    virtual ~TransformComponent() {}

    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    Mat4 const& GetLocalMat4() const {
        return _localTransform;
    }
    Mat4 GetWorldMat4() const {
        if (auto const& parent = _parent.lock()) {
            return _localTransform * parent->GetWorldMat4();
        }
        return _localTransform;
    }

    // TODO make these return const& to memory in Mat4
    Vec3 GetWorldPos() const {
        return GetWorldMat4().GetCol3(3);
    }
    void SetLocalPos(Vec3 const& p) {
        _localTransform._col3 = Vec4(p._x, p._y, p._z, 1.f);
    }
    void SetWorldPos(Vec3 const& p) {
        if (auto const& parent = _parent.lock()) {
            Mat4 parentInv = parent->GetWorldMat4().InverseAffine();
            Vec3 newLocalPos = p + parentInv.GetCol3(3);
            SetLocalPos(newLocalPos);
        } else {
            SetLocalPos(p);
        }
    }
    Vec3 GetLocalXAxis() const {
        return _localTransform.GetCol3(0);
    }
    Vec3 GetLocalYAxis() const {
        return _localTransform.GetCol3(1);
    }
    Vec3 GetLocalZAxis() const {
        return _localTransform.GetCol3(2);
    }
    Vec3 GetLocalPos() const {
        return _localTransform.GetCol3(3);
    }

    Vec3 GetWorldXAxis() const {
        return GetWorldRot().GetCol(0);
    }
    Vec3 GetWorldYAxis() const {
        return GetWorldRot().GetCol(1);
    }
    Vec3 GetWorldZAxis() const {
        return GetWorldRot().GetCol(2);
    }

    Mat3 GetWorldRot() const {
        if (auto const& parent = _parent.lock()) {
            return GetLocalRot() * parent->GetWorldRot();
        }
        return GetLocalRot();
    }
    Mat3 GetLocalRot() const {
        return _localTransform.GetMat3();
    }
    void SetLocalRot(Mat3 const& rot) {
        _localTransform.SetTopLeftMat3(rot);
    }
    void SetWorldRot(Mat3 const& rot) {
        if (auto const& parent = _parent.lock()) {
            Mat4 parentInv = parent->GetWorldMat4().InverseAffine();
            Mat3 newLocalRot = rot * parentInv.GetMat3();
            SetLocalRot(newLocalRot);
        } else {
            SetLocalRot(rot);
        }
    }
    void SetWorldMat4(Mat4 const& m) {
        if (auto const& parent = _parent.lock()) {
            Mat4 parentInv = parent->GetWorldMat4().InverseAffine();
            _localTransform = m * parentInv;
        } else {
            _localTransform = m;
        }
    }

    bool HasParent() const {
        return !_parent.expired();
    }
    // Both of these preserve this transform's world transform.
    void Unparent();
    // Handles re-parenting as well, if previously parented.
    void Parent(std::shared_ptr<TransformComponent const> const& parent);

    Vec3 TransformLocalToWorld(Vec3 const& localVec) const;

    virtual void Save(serial::Ptree pt) const override;
    virtual void Load(serial::Ptree pt) override;

    // Serialize
    // TODO store separate rot and pos
    Mat4 _localTransform;
    std::string _parentEntityName;

    // TODO consider caching the world transform and using a generation ID to know when the parent has changed.
    std::weak_ptr<TransformComponent const> _parent;
};