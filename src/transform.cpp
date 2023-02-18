#include "transform.h"

Vec3 Transform::Pos() const {
    return _mat.GetPos();
}

void Transform::SetPos(Vec3 const& p) {
    _mat.SetTranslation(p);
}

void Transform::SetPosX(float v) {
    _mat._m03 = v;
}

void Transform::SetPosY(float v) {
    _mat._m13 = v;
}

void Transform::SetPosZ(float v) {
    _mat._m23 = v;
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

void Transform::ApplyScale(Vec3 const& v) {
    _scale.ElemWiseMult(v);
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

void Transform::SetFromMat4(Mat4 const& mat4) {
    // First, let's extract scale (and create a rot matrix).
    Mat3 mat3 = mat4.GetMat3();
    _scale._x = mat3._col0.Normalize();
    _scale._y = mat3._col1.Normalize();
    _scale._z = mat3._col2.Normalize();

    Quaternion q;
    q.SetFromRotMat(mat3);
    SetQuat(q);

    _mat.SetTranslation(mat4.GetPos());   
}

Vec3 Transform::GetXAxis() const {
    MaybeUpdateRotMat();
    return _mat.GetCol3(0);
}

Vec3 Transform::GetYAxis() const {
    MaybeUpdateRotMat();
    return _mat.GetCol3(1);
}

Vec3 Transform::GetZAxis() const {
    MaybeUpdateRotMat();
    return _mat.GetCol3(2);
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
