#include "quaternion.h"

Quaternion::Quaternion()
    : _v(0.f, 0.f, 0.f, 1.f) {
}

Quaternion Quaternion::Conjugate() const {
    return Quaternion(Vec4(-_v._x, -_v._y, -_v._z, _v._w));
}

float Quaternion::Magnitude() const {
    return sqrt(Vec4::Dot(_v, _v));
}

Quaternion Quaternion::Inverse() const {
    float m = Magnitude();
    if (m == 0.f) {
        printf("ERROR: Tried to invert a Quaternion with 0 magnitude!\n");
        return Quaternion();
    }
    Quaternion conj = Conjugate();
    conj._v /= m;
    return conj;
}

Quaternion Quaternion::operator*(Quaternion const& rhs) const {
    float x = _v._y*rhs._v._z - _v._z*rhs._v._y + _v._x*rhs._v._w + _v._w*rhs._v._x;
    float y = _v._z*rhs._v._x - _v._x*rhs._v._z + _v._y*rhs._v._w + _v._w*rhs._v._y;
    float z = _v._x*rhs._v._y - _v._y*rhs._v._x + _v._z*rhs._v._w + _v._w*rhs._v._z;
    float w = _v._w*rhs._v._w - _v._x*rhs._v._x - _v._y*rhs._v._y - _v._z*rhs._v._z;
    return Quaternion(Vec4(x, y, z, w));
}

void Quaternion::GetRotMat(Mat3& mat) const {
    // float x2 = _v._x * _v._x;
    // float y2 = _v._y * _v._y;
    // float z2 = _v._z * _v._z;
    // float xy = _v._x * _v._y;
    // float wz = _v._w * _v._z;
    // float xz = _v._x * _v._z;
    // float wy = _v._w * _v._y;
    
    mat._m00 = 1-2*_v._y*_v._y-2*_v._z*_v._z;
    mat._m01 = 2*_v._x*_v._y - 2*_v._z*_v._w;
    mat._m02 = 2*_v._x*_v._z + 2*_v._y*_v._w;

    mat._m10 = 2*_v._x*_v._y + 2*_v._z*_v._w;
    mat._m11 = 1-2*_v._x*_v._x-2*_v._z*_v._z;
    mat._m12 = 2*_v._y*_v._z - 2*_v._x*_v._w;

    mat._m20 = 2*_v._x*_v._z - 2*_v._y*_v._w;
    mat._m21 = 2*_v._y*_v._z + 2*_v._x*_v._w;
    mat._m22 = 1-2*_v._x*_v._x-2*_v._y*_v._y;
}

void Quaternion::GetRotMat(Mat4& mat) const {
    mat._m00 = 1-2*_v._y*_v._y-2*_v._z*_v._z;
    mat._m01 = 2*_v._x*_v._y - 2*_v._z*_v._w;
    mat._m02 = 2*_v._x*_v._z + 2*_v._y*_v._w;

    mat._m10 = 2*_v._x*_v._y + 2*_v._z*_v._w;
    mat._m11 = 1-2*_v._x*_v._x-2*_v._z*_v._z;
    mat._m12 = 2*_v._y*_v._z - 2*_v._x*_v._w;

    mat._m20 = 2*_v._x*_v._z - 2*_v._y*_v._w;
    mat._m21 = 2*_v._y*_v._z + 2*_v._x*_v._w;
    mat._m22 = 1-2*_v._x*_v._x-2*_v._y*_v._y;
}

void Quaternion::SetFromAngleAxis(float angleRad, Vec3 const& axis) {
    float halfAngle = angleRad * 0.5f;
    float s = sin(halfAngle);
    float c = cos(halfAngle);
    _v._x = axis._x * s;
    _v._y = axis._y * s;
    _v._z = axis._z * s;
    _v._w = c;
}

void Quaternion::Save(serial::Ptree pt) const {
    _v.Save(pt);
}

void Quaternion::Load(serial::Ptree pt) {
    _v.Load(pt);
}
