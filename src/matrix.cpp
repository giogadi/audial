#include "matrix.h"

Vec3 Vec3::GetNormalized() const {
    // float length = Length();
    // if (length == 0.f) {
    //     return Vec3(0.f, 0.f, 0.f);
    // } else {
    //     return *this / length;
    // }
    Vec3 n = *this;
    n.Normalize();
    return n;
}

float Vec3::Normalize() {
    float length2 = Length2();
    if (length2 == 0.f) {
        Set(0.f, 0.f, 0.f);
        return 0.f;
    }
    float length = sqrt(length2);
    _x /= length;
    _y /= length;
    _z /= length;
    return length;
}

Mat3 Mat3::FromAxisAngle(Vec3 const& axis, float angleRad) {
    float c = cos(angleRad);
    float c1 = 1.f - c;
    float s = sin(angleRad);
    float x = axis._x;
    float y = axis._y;
    float z = axis._z;
    float xx = axis._x * axis._x;
    float yy = axis._y * axis._y;
    float zz = axis._z * axis._z;
    float xyc1 = x*y*c1;
    float yzc1 = y*z*c1;
    float xzc1 = x*z*c1;
    float xs = x*s;
    float ys = y*s;
    float zs = z*s;
    return Mat3(
        c+xx*c1, xyc1+zs, xzc1-ys,
        xyc1-zs, c+yy*c1, yzc1+xs,
        xzc1+ys, yzc1-xs, c+zz*c1
    );
}

// From https://en.wikipedia.org/wiki/Invertible_matrix#Methods_of_matrix_inversion
bool Mat3::TransposeInverse(Mat3& out) {
    float A = _m11*_m22 - _m12*_m21;
    float B = -(_m10*_m22 - _m12*_m20);
    float C = _m10*_m21 - _m11*_m20;
    
    float det = _m00*A + _m01*B + _m02*C;
    if (det < 0.00001) {
        return false;
    }
    float invDet = 1.f / det;

    out._m00 = A * invDet;
    out._m01 = B * invDet;
    out._m02 = C * invDet;

    out._m10 = -(_m01*_m22 - _m02*_m21) * invDet;
    out._m11 = (_m00*_m22 - _m02*_m20) * invDet;
    out._m12 = -(_m00*_m21 - _m01*_m20) * invDet;

    out._m20 = (_m01*_m12 - _m02*_m11) * invDet;
    out._m21 = -(_m00*_m12 - _m02*_m10) * invDet;
    out._m22 = (_m00*_m11 - _m01*_m10) * invDet;

    return true;
}

Mat4 Mat4::LookAt(Vec3 const& eye, Vec3 const& at, Vec3 const& up) {
    Mat3 r;
    r._col2 = (at - eye).GetNormalized();
    r._col0 = Vec3::Cross(r._col2, up).GetNormalized();
    r._col1 = Vec3::Cross(r._col0, r._col2).GetNormalized();
    
    r._col2 = -r._col2;
    
    Vec3 t = r.MultiplyTranspose(-eye);

    return Mat4(
        r._col0._x, r._col1._x, r._col2._x, 0.f,
        r._col0._y, r._col1._y, r._col2._y, 0.f,
        r._col0._z, r._col1._z, r._col2._z, 0.f,
        t._x, t._y, t._z, 1.f);
}

// https://www.khronos.org/opengl/wiki/GluPerspective_code
Mat4 Mat4::Frustrum(
    float left, float right, float bottom, float top, float znear, float zfar) {

    Mat4 m;
    float* matrix = m._data;
    float temp, temp2, temp3, temp4;
    temp = 2.0 * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    matrix[0] = temp / temp2;
    matrix[1] = 0.0;
    matrix[2] = 0.0;
    matrix[3] = 0.0;
    matrix[4] = 0.0;
    matrix[5] = temp / temp3;
    matrix[6] = 0.0;
    matrix[7] = 0.0;
    matrix[8] = (right + left) / temp2;
    matrix[9] = (top + bottom) / temp3;
    matrix[10] = (-zfar - znear) / temp4;
    matrix[11] = -1.0;
    matrix[12] = 0.0;
    matrix[13] = 0.0;
    matrix[14] = (-temp * zfar) / temp4;
    matrix[15] = 0.0;
    return m;
}

Mat4 Mat4::Perspective(
    float fovyRadians, float aspectRatio,
    float znear, float zfar) {
    
    float ymax, xmax;
    float temp, temp2, temp3, temp4;
    ymax = znear * tanf(0.5f * fovyRadians);
    // ymin = -ymax;
    // xmin = -ymax * aspectRatio;
    xmax = ymax * aspectRatio;
    return Frustrum(-xmax, xmax, -ymax, ymax, znear, zfar);
}

void Mat4::Scale(float x) {
    _m00 *= x;
    _m01 *= x;
    _m02 *= x;
    _m10 *= x;
    _m11 *= x;
    _m12 *= x;
    _m20 *= x;
    _m21 *= x;
    _m22 *= x;
}