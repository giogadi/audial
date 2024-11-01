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
    float xx = _v._x * _v._x;
    float yy = _v._y * _v._y;
    float zz = _v._z * _v._z;
    float xy = _v._x * _v._y;
    float xz = _v._x * _v._z;
    float xw = _v._x * _v._w;
    float yz = _v._y * _v._z;
    float yw = _v._y * _v._w;
    float zw = _v._z * _v._w;

    // MANUALLY INLINE THESE PLEASE    
    mat(0,0) = 1 - 2*(yy + zz);
    mat(0,1) = 2*(xy - zw);
    mat(0,2) = 2*(xz + yw);
    
    mat(1,0) = 2*(xy + zw);
    mat(1,1) = 1 - 2*(xx + zz);
    mat(1,2) = 2*(yz - xw);

    mat(2,0) = 2*(xz - yw);
    mat(2,1) = 2*(yz + xw);
    mat(2,2) = 1-2*(xx + yy);
}

void Quaternion::GetRotMat(Mat4& mat) const {
    float xx = _v._x * _v._x;
    float yy = _v._y * _v._y;
    float zz = _v._z * _v._z;
    float xy = _v._x * _v._y;
    float xz = _v._x * _v._z;
    float xw = _v._x * _v._w;
    float yz = _v._y * _v._z;
    float yw = _v._y * _v._w;
    float zw = _v._z * _v._w;

    // MANUALLY INLINE THESE PLEASE    
    mat(0,0) = 1 - 2*(yy + zz);
    mat(0,1) = 2*(xy - zw);
    mat(0,2) = 2*(xz + yw);
    
    mat(1,0) = 2*(xy + zw);
    mat(1,1) = 1 - 2*(xx + zz);
    mat(1,2) = 2*(yz - xw);

    mat(2,0) = 2*(xz - yw);
    mat(2,1) = 2*(yz + xw);
    mat(2,2) = 1-2*(xx + yy);
}

void Quaternion::SetFromRotMat(Mat3 const& matTranspose) {

    // TODO GET RID OF THIS TRANSPOSE
    Mat3 const mat = matTranspose.GetTranspose();
    
    // Determine which of w, x, y, or z has the largest absolute value
    float fourWSquaredMinus1 = mat(0,0) + mat(1,1) + mat(2,2);
    float fourXSquaredMinus1 = mat(0,0) - mat(1,1) - mat(2,2);
    float fourYSquaredMinus1 = mat(1,1) - mat(0,0) - mat(2,2);
    float fourZSquaredMinus1 = mat(2,2) - mat(0,0) - mat(1,1);

    int biggestIndex = 0;
    float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
    if (fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourXSquaredMinus1;
        biggestIndex = 1;
    }
    if (fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourYSquaredMinus1;
        biggestIndex = 2;
    }
    if (fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourZSquaredMinus1;
        biggestIndex = 3;
    }

    // Perform square root and division
    float biggestVal = sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
    float mult = 0.25f / biggestVal;

    // Apply table to compute quaternion values
    switch (biggestIndex) {
        case 0:
            _v._w = biggestVal;
            _v._x = (mat(1,2) - mat(2,1)) * mult;
            _v._y = (mat(2,0) - mat(0,2)) * mult;
            _v._z = (mat(0,1) - mat(1,0)) * mult;
            break;

        case 1:
            _v._x = biggestVal;
            _v._w = (mat(1,2) - mat(2,1)) * mult;
            _v._y = (mat(0,1) + mat(1,0)) * mult;
            _v._z = (mat(2,0) + mat(0,2)) * mult;
            break;

        case 2:
            _v._y = biggestVal;
            _v._w = (mat(2,0) - mat(0,2)) * mult;
            _v._x = (mat(0,1) + mat(1,0)) * mult;
            _v._z = (mat(1,2) + mat(2,1)) * mult;
            break;

        case 3:
            _v._z = biggestVal;
            _v._w = (mat(0,1) - mat(1,0)) * mult;
            _v._x = (mat(2,0) + mat(0,2)) * mult;
            _v._y = (mat(1,2) + mat(2,1)) * mult;
            break;
    }
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

Vec3 Quaternion::EulerAngles() const {
    Vec3 euler;

    // Extract sin(pitch)
    float sp = -2.0f * (_v._y*_v._z - _v._w*_v._x);

    // Check for Gimbal lock, giving slight tolerance
    // for numerical imprecision
    if (fabs(sp) > 0.9999f) {

        // Looking straight up or down
        euler._x = 1.570796f * sp; // pi/2

        // Compute heading, slam bank to zero
        euler._y = atan2(-_v._x*_v._z + _v._w*_v._y, 0.5f - _v._y*_v._y - _v._z*_v._z);
        euler._z = 0.0f;

    } else {

        // Compute angles
        euler._x = asin(sp);
        euler._y = atan2(_v._x*_v._z + _v._w*_v._y, 0.5f - _v._x*_v._x - _v._y*_v._y);
        euler._z = atan2(_v._x*_v._y + _v._w*_v._z, 0.5f - _v._x*_v._x - _v._z*_v._z);
    }

    return euler;
}

void Quaternion::SetFromEulerAngles(Vec3 const& e) {
    float halfX = e._x * 0.5f;
    Quaternion x(Vec4(sin(halfX), 0.f, 0.f, cos(halfX)));

    float halfY = e._y * 0.5f;
    Quaternion y(Vec4(0.f, sin(halfY), 0.f, cos(halfY)));

    float halfZ = e._z * 0.5f;
    Quaternion z(Vec4(0.f, 0.f, sin(halfZ), cos(halfZ)));

    Quaternion result = y * x * z;
    _v = result._v;
}

void Quaternion::Save(serial::Ptree pt) const {
    _v.Save(pt);
}

void Quaternion::Load(serial::Ptree pt) {
    _v.Load(pt);
}
