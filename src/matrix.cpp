#include "matrix.h"

Vec3 Vec3::GetNormalized() const {
    float length = Length();
    if (length == 0.f) {
        return Vec3(0.f, 0.f, 0.f);
    } else {
        return *this / length;
    }
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