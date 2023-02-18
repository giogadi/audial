#pragma once

#include <cmath>
#include <cassert>

#include "serial.h"

struct Vec3 {
    constexpr Vec3()
        : _x(0.), _y(0.), _z(0.) {}
    constexpr Vec3(float x, float y, float z)
        : _x(x), _y(y), _z(z) {}
    constexpr explicit Vec3(float* v)
        : _x(v[0]), _y(v[1]), _z(v[2]) {}

    void Set(float x, float y, float z) {
        _x = x;
        _y = y;
        _z = z;
    }

    static inline float Dot(Vec3 const& lhs, Vec3 const& rhs) {
        return lhs._x*rhs._x + lhs._y*rhs._y + lhs._z*rhs._z;
    }

    static inline Vec3 Cross(Vec3 const& a, Vec3 const& b) {
        return Vec3(
            a._y*b._z - a._z*b._y,
            a._z*b._x - a._x*b._z,
            a._x*b._y - a._y*b._x);
    }

    float Length2() const {
        return Dot(*this, *this);
    }
    float Length() const {
        return sqrt(Length2());
    }

    // Returns 0,0,0 if input vector is 0.
    Vec3 GetNormalized() const;
    // Return length before normalizing.
    float Normalize();

    float IsZero() const {
        return _x == 0.f && _y == 0.f && _z == 0.f;
    }

    void ElemWiseMult(Vec3 const& v) {
        _x *= v._x;
        _y *= v._y;
        _z *= v._z;
    }

    float& operator()(int i) {
        switch (i) {
            case 0: return _x;
            case 1: return _y;
            case 2: return _z;
            default: {
                assert(false);
                return _x;
            }
        }
    }
    float const operator()(int i) const {
        switch (i) {
            case 0: return _x;
            case 1: return _y;
            case 2: return _z;
            default: {
                assert(false);
                return 0.f;
            }
        }
    }
    Vec3& operator+=(Vec3 const& rhs) {
        _x += rhs._x;
        _y += rhs._y;
        _z += rhs._z;
        return *this;
    }
    Vec3& operator-=(Vec3 const& rhs) {
        _x -= rhs._x;
        _y -= rhs._y;
        _z -= rhs._z;
        return *this;
    }
    Vec3& operator*=(float a) {
        _x *= a;
        _y *= a;
        _z *= a;
        return *this;
    }

    void CopyToArray(float* v) {
        v[0] = _x;
        v[1] = _y;
        v[2] = _z;
    }

    void Save(serial::Ptree pt) const {
        pt.PutFloat("x", _x);
        pt.PutFloat("y", _y);
        pt.PutFloat("z", _z);
    }
    void Load(serial::Ptree pt) {
        _x = pt.GetFloat("x");
        _y = pt.GetFloat("y");
        _z = pt.GetFloat("z");
    }

    float _x, _y, _z;
};

inline Vec3 operator+(Vec3 const& lhs, Vec3 const& rhs) {
    return Vec3(lhs._x + rhs._x, lhs._y + rhs._y, lhs._z + rhs._z);
}

inline Vec3 operator-(Vec3 const& lhs, Vec3 const& rhs) {
    return Vec3(lhs._x - rhs._x, lhs._y - rhs._y, lhs._z - rhs._z);
}

inline Vec3 operator-(Vec3 const& v) {
    return Vec3(-v._x, -v._y, -v._z);
}

inline Vec3 operator*(Vec3 const& v, float const a) {
    return Vec3(a*v._x, a*v._y, a*v._z);
}

inline Vec3 operator*(float const a, Vec3 const& v) {
    return v * a;
}
inline Vec3 operator/(Vec3 const& v, float const a) {
    return Vec3(v._x / a, v._y / a, v._z / a);
}

struct Mat3 {
    // TODO: transpose this
    constexpr Mat3(float m00, float m10, float m20,
                   float m01, float m11, float m21,
                   float m02, float m12, float m22) :
        _data{m00, m10, m20, m01, m11, m21, m02, m12, m22} {}
    constexpr Mat3()
        : Mat3(1.f, 0.f, 0.f,
               0.f, 1.f, 0.f,
               0.f, 0.f, 1.f) {}

    static Mat3 FromAxisAngle(Vec3 const& axis, float angleRad);

    float const operator()(int r, int c) const {
        assert(r >= 0 && r < 3 && c >= 0 && c < 3);
        return _data[3*c + r];
    }
    float& operator()(int r, int c) {
        assert(r >= 0 && r < 3 && c >= 0 && c < 3);
        return _data[3*c + r];
    }

    Vec3 GetCol(int c) const {
        assert(c >= 0 && c < 3);
        int colStart = 3*c;
        return Vec3(_data[colStart], _data[colStart + 1], _data[colStart + 2]);
    }
    void SetCol(int c, Vec3 const& v) {
        assert(c >= 0 && c < 3);
        int colStart = 3*c;
        _data[colStart] = v._x;
        _data[colStart+1] = v._y;
        _data[colStart+2] = v._z;
    }

    Vec3 GetRow(int r) const {
        assert(r >= 0 && r < 3);
        return Vec3(_data[r], _data[r + 3], _data[r + 6]);
    }
    void SetRow(int r, Vec3 const& v) {
        assert(r >= 0 && r < 3);
        _data[r] = v._x;
        _data[r+3] = v._y;
        _data[r+6] = v._z;
    }

    Mat3 GetTranspose() const {
        return Mat3(
            _data[0], _data[3], _data[6],
            _data[1], _data[4], _data[7],
            _data[2], _data[5], _data[8]);
    }

    Vec3 MultiplyTranspose(Vec3 const& v) const {
        return Vec3(
            _data[0]*v._x + _data[1]*v._y + _data[2]*v._z,
            _data[3]*v._x + _data[4]*v._y + _data[5]*v._z,
            _data[6]*v._x + _data[7]*v._y + _data[8]*v._z);
    }

    // Takes inverse then transpose of this matrix and returns result in "out".
    bool TransposeInverse(Mat3& out) const;
    
    float _data[9];
};

inline Vec3 operator*(Mat3 const& mat, Vec3 const& v) {
    float const* m = mat._data;
    return Vec3(
        m[0]*v._x + m[3]*v._y + m[6]*v._z,
        m[1]*v._x + m[4]*v._y + m[7]*v._z,
        m[2]*v._x + m[5]*v._y + m[8]*v._z);
}

inline Mat3 operator*(Mat3 const& a_mat, Mat3 const& b_mat) {
    float const* a = a_mat._data;
    float const* b = b_mat._data;
    return Mat3(
        a[0]*b[0] + a[3]*b[1] + a[6]*b[2],
        a[1]*b[0] + a[4]*b[1] + a[7]*b[2],
        a[2]*b[0] + a[5]*b[1] + a[8]*b[2],

        a[0]*b[3] + a[3]*b[4] + a[6]*b[5],
        a[1]*b[3] + a[4]*b[4] + a[7]*b[5],
        a[2]*b[3] + a[5]*b[4] + a[8]*b[5],

        a[0]*b[6] + a[3]*b[7] + a[6]*b[8],
        a[1]*b[6] + a[4]*b[7] + a[7]*b[8],
        a[2]*b[6] + a[5]*b[7] + a[8]*b[8]
    );
}

struct Vec4 {
    constexpr Vec4()
        : _x(0.), _y(0.), _z(0.), _w(0.f) {}
    constexpr Vec4(float x, float y, float z, float w)
        : _x(x), _y(y), _z(z), _w(w) {}
    constexpr explicit Vec4(float* v)
        : _x(v[0]), _y(v[1]), _z(v[2]), _w(v[3]) {}
    
    void Set(float x, float y, float z, float w) {
        _x = x;
        _y = y;
        _z = z;
        _w = w;
    }

    static float Dot(Vec4 const& a, Vec4 const& b) {
        return a._x*b._x + a._y*b._y + a._z*b._z + a._w*b._w;
    }
    void ElemWiseMult(Vec4 const& v) {
        _x *= v._x;
        _y *= v._y;
        _z *= v._z;
        _w *= v._w;
    }

    bool operator==(Vec4 const& rhs) const {
        return _x == rhs._x && _y == rhs._y && _z == rhs._z && _w == rhs._w;
    }
    bool operator!=(Vec4 const& rhs) const {
        return !(*this == rhs);
    }
    Vec4& operator*=(float const rhs) {
        _x *= rhs;
        _y *= rhs;
        _z *= rhs;
        _w *= rhs;
        return *this;
    }
    Vec4& operator/=(float const rhs) {
        _x /= rhs;
        _y /= rhs;
        _z /= rhs;
        _w /= rhs;
        return *this;
    }

    void CopyToArray(float* v) {
        v[0] = _x;
        v[1] = _y;
        v[2] = _z;
        v[3] = _w;
    }

    void Save(serial::Ptree pt) const {
        pt.PutFloat("x", _x);
        pt.PutFloat("y", _y);
        pt.PutFloat("z", _z);
        pt.PutFloat("w", _w);
    }
    void Load(serial::Ptree pt) {
        _x = pt.GetFloat("x");
        _y = pt.GetFloat("y");
        _z = pt.GetFloat("z");
        _w = pt.GetFloat("w");
    }
    
    float _x, _y, _z, _w;
};

inline Vec4 operator+(Vec4 const& lhs, Vec4 const& rhs) {
    return Vec4(lhs._x + rhs._x, lhs._y + rhs._y, lhs._z + rhs._z, lhs._w + rhs._w);
}

inline Vec4 operator-(Vec4 const& lhs, Vec4 const& rhs) {
    return Vec4(lhs._x - rhs._x, lhs._y - rhs._y, lhs._z - rhs._z, lhs._w - rhs._w);
}

inline Vec4 operator-(Vec4 const& v) {
    return Vec4(-v._x, -v._y, -v._z, -v._w);
}

inline Vec4 operator*(Vec4 const& v, float const a) {
    return Vec4(a*v._x, a*v._y, a*v._z, a*v._w);
}

inline Vec4 operator*(float const a, Vec4 const& v) {
    return v * a;
}
inline Vec4 operator/(Vec4 const& v, float const a) {
    return Vec4(v._x / a, v._y / a, v._z / a, v._w / a);
}

struct Mat4 {
    // TODO: transpose this
    constexpr Mat4(float m00, float m10, float m20, float m30,
                   float m01, float m11, float m21, float m31,
                   float m02, float m12, float m22, float m32,
                   float m03, float m13, float m23, float m33) :
        _data{m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33} {}
    constexpr Mat4()
        : Mat4(1.f, 0.f, 0.f, 0.f,
               0.f, 1.f, 0.f, 0.f,
               0.f, 0.f, 1.f, 0.f,
               0.f, 0.f, 0.f, 1.f) {}

    float const operator()(int r, int c) const {
        assert(r >= 0 && r < 4 && c >= 0 && c < 4);
        return _data[4*c + r];
    }
    float& operator()(int r, int c) {
        assert(r >= 0 && r < 4 && c >= 0 && c < 4);
        return _data[4*c + r];
    }    

    Vec4 GetCol(int c) const {
        assert(c >= 0 && c < 4);
        int colStart = 4*c;
        return Vec4(_data[colStart], _data[colStart + 1], _data[colStart + 2], _data[colStart + 3]);
    }
    void SetCol(int c, Vec4 const& v) {
        assert(c >= 0 && c < 4);
        int colStart = 4*c;
        _data[colStart] = v._x;
        _data[colStart+1] = v._y;
        _data[colStart+2] = v._z;
        _data[colStart+3] = v._w;
    }

    Vec4 GetRow(int r) const {
        assert(r >= 0 && r < 4);
        return Vec4(_data[r], _data[r+4], _data[r+8], _data[r+12]);
    }
    void SetRow(int r, Vec4 const& v) {
        assert(r >= 0 && r < 4);
        _data[r] = v._x;
        _data[r+4] = v._y;
        _data[r+8] = v._z;
        _data[r+12] = v._w;
    }

    Vec3 GetCol3(int c) const {
        assert(c >= 0 && c < 4);
        int colStart = 4*c;
        return Vec3(_data[colStart], _data[colStart + 1], _data[colStart + 2]);
    }
    void SetCol3(int c, Vec3 const& v) {
        assert(c >= 0 && c < 4);
        int colStart = 4*c;
        _data[colStart] = v._x;
        _data[colStart+1] = v._y;
        _data[colStart+2] = v._z;
    }

    Vec3 GetRow3(int r) const {
        assert(r >= 0 && r < 4);
        return Vec3(_data[r], _data[r+4], _data[r+8]);
    }
    void SetRow3(int r, Vec3 const& v) {
        assert(r >= 0 && r < 4);
        _data[r] = v._x;
        _data[r+4] = v._y;
        _data[r+8] = v._z;
    }

    Mat4 GetTranspose() const {
        return Mat4(
            _data[0], _data[4], _data[8], _data[12],
            _data[1], _data[5], _data[9], _data[13],
            _data[2], _data[6], _data[10], _data[14],
            _data[3], _data[7], _data[11], _data[15]);
    }

    Mat3 GetMat3() const {
        return Mat3(
            _data[0], _data[1], _data[2],
            _data[4], _data[5], _data[6],
            _data[8], _data[9], _data[10]);
    }
    
    void SetTopLeftMat3(Mat3 const& mat3) {
        float const* m3 = mat3._data;
        _data[0] = m3[0];
        _data[1] = m3[1];
        _data[2] = m3[2];
        _data[4] = m3[3];
        _data[5] = m3[4];
        _data[6] = m3[5];
        _data[8] = m3[6];
        _data[9] = m3[7];
        _data[10] = m3[8];
    }

    // These APPLY the given scale to the current matrix.
    // NOTE!!!! These only apply to the 3x3 rot matrix
    void ScaleUniform(float x);
    void Scale(float x, float y, float z);

    void Translate(Vec3 const& t) {
        _data[12] += t._x;
        _data[13] += t._y;
        _data[14] += t._z;
    }

    void SetTranslation(Vec3 const& t) {
        _data[12] = t._x;
        _data[13] = t._y;
        _data[14] = t._z;
    }

    Vec3 GetPos() const {
        return Vec3(_data[12], _data[13], _data[14]);
    }

    // Assumes the mat is just a simple rotation and translation. No scaling.
    Mat4 InverseAffine() const {
        Mat4 inverse;
        inverse.SetTopLeftMat3(GetMat3().GetTranspose());
        Vec3 newPos = -(inverse.GetMat3() * GetCol3(3));
        inverse.SetTranslation(newPos);
        return inverse;
    }

    static Mat4 LookAt(Vec3 const& eye, Vec3 const& at, Vec3 const& up);
    static Mat4 Frustrum(float left, float right, float bottom, float top, float znear, float zfar);
    static Mat4 Perspective(float fovyRadians, float aspectRatio, float znear, float zfar);
    static Mat4 Ortho(float width, float aspect, float zNear, float zFar);
    static Mat4 Ortho(float left, float right, float top, float bottom, float zNear, float zFar);

    void Save(serial::Ptree pt) const {
        char name[] = "mXX";
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                snprintf(name, 4, "m%d%d", j, i);
                pt.PutFloat(name, _data[4*i + j]);
            }
        }
    }
    void Load(serial::Ptree pt) {
        char name[] = "mXX";
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                snprintf(name, 4, "m%d%d", j, i);
                _data[4*i + j] = pt.GetFloat(name);
            }
        }
    }
    
    float _data[16];
};

inline Mat4 operator*(Mat4 const& mat_a, Mat4 const& mat_b) {
    float const* a = mat_a._data;
    float const* b = mat_b._data;
    return Mat4(
        a[0]*b[0] + a[4]*b[1] + a[8]*b[2] + a[12]*b[3],
        a[1]*b[0] + a[5]*b[1] + a[9]*b[2] + a[13]*b[3],
        a[2]*b[0] + a[6]*b[1] + a[10]*b[2] + a[14]*b[3],
        a[3]*b[0] + a[7]*b[1] + a[11]*b[2] + a[15]*b[3],

        a[0]*b[4] + a[4]*b[5] + a[8]*b[6] + a[12]*b[7],
        a[1]*b[4] + a[5]*b[5] + a[9]*b[6] + a[13]*b[7],
        a[2]*b[4] + a[6]*b[5] + a[10]*b[6] + a[14]*b[7],
        a[3]*b[4] + a[7]*b[5] + a[11]*b[6] + a[15]*b[7],

        a[0]*b[8] + a[4]*b[9] + a[8]*b[10] + a[12]*b[11],
        a[1]*b[8] + a[5]*b[9] + a[9]*b[10] + a[13]*b[11],
        a[2]*b[8] + a[6]*b[9] + a[10]*b[10] + a[14]*b[11],
        a[3]*b[8] + a[7]*b[9] + a[11]*b[10] + a[15]*b[11],

        a[0]*b[12] + a[4]*b[13] + a[8]*b[14] + a[12]*b[15],
        a[1]*b[12] + a[5]*b[13] + a[9]*b[14] + a[13]*b[15],
        a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15],
        a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15]
    );
}

inline Vec4 operator*(Mat4 const& mat, Vec4 const& v) {
    float const* m = mat._data;
    return Vec4(
        m[0]*v._x + m[4]*v._y + m[8]*v._z + m[12]*v._w,
        m[1]*v._x + m[5]*v._y + m[9]*v._z + m[13]*v._w,
        m[2]*v._x + m[6]*v._y + m[10]*v._z + m[14]*v._w,
        m[3]*v._x + m[7]*v._y + m[11]*v._z + m[15]*v._w);
}
