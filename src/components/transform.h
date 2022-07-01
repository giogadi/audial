#pragma once

#include "component.h"
#include "matrix.h"

class TransformComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::Transform; }
    TransformComponent()
        : _transform(Mat4::Identity()) {}

    virtual ~TransformComponent() {}

    // TODO make these return const& to memory in Mat4
    Vec3 GetPos() const {
        return _transform.GetCol3(3);
    }
    void SetPos(Vec3 const& p) {
        _transform._col3 = Vec4(p._x, p._y, p._z, 1.f);
    }
    Vec3 GetXAxis() const {
        return _transform.GetCol3(0);
    }
    Vec3 GetYAxis() const {
        return _transform.GetCol3(1);
    }
    Vec3 GetZAxis() const {
        return _transform.GetCol3(2);
    }
    Mat4 GetMat4() const {
        return _transform;
    }

    Mat3 GetRot() const {
        return _transform.GetMat3();
    }
    void SetRot(Mat3 const& rot) {
        _transform.SetTopLeftMat3(rot);
    }

    virtual void Save(ptree& pt) const override;
    virtual void Load(ptree const& pt) override;

    // TODO store separate rot and pos
    Mat4 _transform;
};