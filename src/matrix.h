#pragma once

#include <cmath>
#include <cassert>

#include "serial.h"

struct Vec3 {
    Vec3()
        : _x(0.), _y(0.), _z(0.) {}
    Vec3(float x, float y, float z)
        : _x(x), _y(y), _z(z) {}

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

    float& operator[](int i) {
        return _data[i];
    }
    float const& operator[](int i) const {
        return _data[i];
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

    union {
        float _data[3];
        struct {
            float _x, _y, _z;
        };
    };
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
    Mat3() {
        *this = Identity();
    }
    Mat3(float m00, float m10, float m20,
         float m01, float m11, float m21,
         float m02, float m12, float m22)
        : _m00(m00), _m10(m10), _m20(m20),
          _m01(m01), _m11(m11), _m21(m21),
          _m02(m02), _m12(m12), _m22(m22) {}
    Mat3(Vec3 const& col0, Vec3 const& col1, Vec3 const& col2)
        : _col0(col0), _col1(col1), _col2(col2) {}

    static Mat3 Identity() {
        return Mat3(
            1.f, 0.f, 0.f,
            0.f, 1.f, 0.f,
            0.f, 0.f, 1.f);
    }

    static Mat3 FromAxisAngle(Vec3 const& axis, float angleRad);

    Mat3 GetTranspose() const {
        return Mat3(
            _m00, _m01, _m02,
            _m10, _m11, _m12,
            _m20, _m21, _m22);
    }

    Vec3 MultiplyTranspose(Vec3 const& v) const {
        return Vec3(
            _m00*v._x + _m10*v._y + _m20*v._z,
            _m01*v._x + _m11*v._y + _m21*v._z,
            _m02*v._x + _m12*v._y + _m22*v._z);
    }

    Vec3& GetCol(int c) {
        return const_cast<Vec3&>(static_cast<Mat3 const&>(*this).GetCol(c));
    }
    Vec3 const& GetCol(int c) const {
        switch (c) {
            case 0: return _col0;
            case 1: return _col1;
            case 2: return _col2;
        }
        assert(false);
        return _col0;
    }

    // Takes inverse then transpose of this matrix and returns result in "out".
    bool TransposeInverse(Mat3& out);

    union {
        float _data[9];
        struct {
            // Data is laid out so that column vectors are contiguous in memory. index names are mathematical [row,column].
            float
                _m00, _m10, _m20,
                _m01, _m11, _m21,
                _m02, _m12, _m22;
        };
        struct {
            Vec3 _col0, _col1, _col2;
        };
    };
};

inline Vec3 operator*(Mat3 const& m, Vec3 const& v) {
    return Vec3(
        m._m00*v._x + m._m01*v._y + m._m02*v._z,
        m._m10*v._x + m._m11*v._y + m._m12*v._z,
        m._m20*v._x + m._m21*v._y + m._m22*v._z);
}

inline Mat3 operator*(Mat3 const& a, Mat3 const& b) {
    return Mat3(
        a._m00*b._m00 + a._m01*b._m10 + a._m02*b._m20,
        a._m10*b._m00 + a._m11*b._m10 + a._m12*b._m20,
        a._m20*b._m00 + a._m21*b._m10 + a._m22*b._m20,

        a._m00*b._m01 + a._m01*b._m11 + a._m02*b._m21,
        a._m10*b._m01 + a._m11*b._m11 + a._m12*b._m21,
        a._m20*b._m01 + a._m21*b._m11 + a._m22*b._m21,

        a._m00*b._m02 + a._m01*b._m12 + a._m02*b._m22,
        a._m10*b._m02 + a._m11*b._m12 + a._m12*b._m22,
        a._m20*b._m02 + a._m21*b._m12 + a._m22*b._m22
    );
}

struct Vec4 {
    constexpr Vec4()
        : _x(0.), _y(0.), _z(0.), _w(0.f) {}
    constexpr Vec4(float x, float y, float z, float w)
        : _x(x), _y(y), _z(z), _w(w) {}
    
    void Set(float x, float y, float z, float w) {
        _x = x;
        _y = y;
        _z = z;
        _w = w;
    }
    union {
        float _data[4];
        struct {
            float _x, _y, _z, _w;
        };
        struct {
            float _r, _g, _b, _a;
        };
    };
    static float Dot(Vec4 const& a, Vec4 const& b) {
        return a._x*b._x + a._y*b._y + a._z*b._z + a._w*b._w;
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

    bool operator==(Vec4 const& rhs) const {
        return _x == rhs._x && _y == rhs._y && _z == rhs._z && _w == rhs._w;
    }
    bool operator!=(Vec4 const& rhs) const {
        return !(*this == rhs);
    }
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
    Mat4() {
        *this = Identity();
    }
    Mat4(float m00, float m10, float m20, float m30,
         float m01, float m11, float m21, float m31,
         float m02, float m12, float m22, float m32,
         float m03, float m13, float m23, float m33)
        : _m00(m00), _m10(m10), _m20(m20), _m30(m30),
          _m01(m01), _m11(m11), _m21(m21), _m31(m31),
          _m02(m02), _m12(m12), _m22(m22), _m32(m32),
          _m03(m03), _m13(m13), _m23(m23), _m33(m33) {}

    static Mat4 Identity() {
        return Mat4(
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f);
    }
    static Mat4 Zero() {
        return Mat4(
            0.f, 0.f, 0.f, 0.f,
            0.f, 0.f, 0.f, 0.f,
            0.f, 0.f, 0.f, 0.f,
            0.f, 0.f, 0.f, 0.f);
    }

    Mat3 GetMat3() const {
        return Mat3(
            _m00, _m10, _m20,
            _m01, _m11, _m21,
            _m02, _m12, _m22);
    }

    Vec4 GetRow(int r) const {
        return Vec4(_data[r], _data[r+4], _data[r+8], _data[r+12]);
    }
    Vec4& GetCol(int c) {
        return const_cast<Vec4&>(static_cast<Mat4 const&>(*this).GetCol(c));
    }
    Vec4 const& GetCol(int c) const {
        switch (c) {
            case 0: return _col0;
            case 1: return _col1;
            case 2: return _col2;
            case 3: return _col3;
        }
        assert(false);
        return _col0;
    }

    // TODO: maybe add a version that just reinterpets the memory instead of copying
    Vec3 GetCol3(int c) const {
        assert(c < 4);
        return Vec3(_data[4*c], _data[4*c + 1], _data[4*c + 2]);
    }

    void SetTopLeftMat3(Mat3 const& m3) {
        for (int c = 0; c < 3; ++c) {
            Vec4& v4 = this->GetCol(c);
            Vec3 const& v3 = m3.GetCol(c);
            v4._x = v3._x;
            v4._y = v3._y;
            v4._z = v3._z;
        }
    }

    // NOTE!!!! These only apply to the 3x3 rot matrix
    void ScaleUniform(float x);
    void Scale(float x, float y, float z);

    // Assumes the mat is just a simple rotation and translation. No scaling.
    Mat4 InverseAffine() const {
        Mat4 inverse = Mat4::Identity();
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
    
    void Translate(Vec3 const& t) {
        _m03 += t._x;
        _m13 += t._y;
        _m23 += t._z;
    }

    void SetTranslation(Vec3 const& t) {
        _m03 = t._x;
        _m13 = t._y;
        _m23 = t._z;
    }
    
    Vec3 GetXAxis() const {
        return GetCol3(0);
    }
    Vec3 GetYAxis() const {
        return GetCol3(1);
    }
    Vec3 GetZAxis() const {
        return GetCol3(2);
    }
    Vec3 GetPos() const {
        return GetCol3(3);
    }

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

    union {
        float _data[16];
        struct {
            // Data is laid out so that column vectors are contiguous in memory. index names are mathematical [row,column].
            float
                _m00, _m10, _m20, _m30,
                _m01, _m11, _m21, _m31,
                _m02, _m12, _m22, _m32,
                _m03, _m13, _m23, _m33;
        };
        struct {
            Vec4 _col0, _col1, _col2, _col3;
        };
    };
};

inline Mat4 operator*(Mat4 const& a, Mat4 const& b) {
    Mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float val = Vec4::Dot(a.GetRow(j), b.GetCol(i));
            result._data[4*i + j] = val;
        }
    }
    return result;
}

inline Vec4 operator*(Mat4 const& m, Vec4 const& v) {
    return Vec4(
        m._m00*v._x + m._m01*v._y + m._m02*v._z + m._m03*v._w,
        m._m10*v._x + m._m11*v._y + m._m12*v._z + m._m13*v._w,
        m._m20*v._x + m._m21*v._y + m._m22*v._z + m._m23*v._w,
        m._m30*v._x + m._m31*v._y + m._m32*v._z + m._m33*v._w);
}

// struct Transform {
//     Transform() {}
//     Transform(Mat3 const& rot, Vec3 const& p)
//         : _rot(rot)
//         , _pos(p) {}

//     Mat4 MakeMat4() const {
//         Mat4 m = Mat4::Identity();
//         for (int c = 0; c < 3; ++c) {
//             Vec4& v4 = m.GetCol(c);
//             Vec3 const& v3 = _rot.GetCol(c);
//             v4._x = v3._x;
//             v4._y = v3._y;
//             v4._z = v3._z;
//         }
//         Vec4& p4 = m.GetCol(3);
//         p4._x = _pos._x;
//         p4._y = _pos._y;
//         p4._z = _pos._z;
//         return m;
//     }

//     Mat3 _rot;
//     Vec3 _pos;
//     // float _scale;
// };
