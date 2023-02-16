#include "transform.h"

Vec3 Transform::Pos() const {
    return _mat.GetPos();
}

void Transform::SetPos(Vec3 const& p) {
    _mat.SetTranslation(p);
}

void Transform::SetQuat(Quaternion const& q) {
    // debug
    float m = q.Magnitude();
    if (std::abs(m - 1.f) > 0.01f) {
        printf("ERROR: Transform given a non-unit quaternion! (%f %f %f %f)\n",
               q._v._x, q._v._y, q._v._z, q._v._w);
    }
    _q = q;
    _rotMatDirty = true;
}

void Transform::MaybeUpdateRotMat() const {
    if (!_rotMatDirty) {
        return;
    }
    _q.GetRotMat(_mat);
    _rotMatDirty = false;
}

Mat4 const& Transform::Mat4NoScale() const {
    MaybeUpdateRotMat();
    return _mat;
}

Mat4 Transform::Mat4Scale() const {
    MaybeUpdateRotMat();
    Mat4 matWithScale = _mat;
    matWithScale.Scale(_scale._x, _scale._y, _scale._z);
    return matWithScale;
}

void Transform::Save(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "pos", _mat.GetPos());
    serial::SaveInNewChildOf(pt, "quat", _q);
    serial::SaveInNewChildOf(pt, "scale", _scale);
}

void Transform::Load(serial::Ptree pt) {
    Vec3 p;
    bool success = LoadFromChildOf(pt, "pos", p);
    assert(success);
    _mat.SetTranslation(p);

    success = LoadFromChildOf(pt, "quat", _q);
    assert(success);

    success = LoadFromChildOf(pt, "scale", _scale);

    _rotMatDirty = true;
}
