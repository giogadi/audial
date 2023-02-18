#include "matrix_new.h"

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
bool Mat3::TransposeInverse(Mat3& out) const {
    Mat3 const& m = *this;
    
    float A = m(1,1)*m(2,2) - m(1,2)*m(2,1);
    float B = -(m(1,0)*m(2,2) - m(1,2)*m(2,0));
    float C = m(1,0)*m(2,1) - m(1,1)*m(2,0);
    
    float det = m(0,0)*A + m(0,1)*B + m(0,2)*C;
    if (det < 0.00001) {
        return false;
    }
    float invDet = 1.f / det;

    // TODO manually inline these
    out(0,0) = A * invDet;
    out(0,1) = B * invDet;
    out(0,2) = C * invDet;

    out(1,0) = -(m(0,1)*m(2,2) - m(0,2)*m(2,1)) * invDet;
    out(1,1) = (m(0,0)*m(2,2) - m(0,2)*m(2,0)) * invDet;
    out(1,2) = -(m(0,0)*m(2,1) - m(0,1)*m(2,0)) * invDet;

    out(2,0) = (m(0,1)*m(1,2) - m(0,2)*m(1,1)) * invDet;
    out(2,1) = -(m(0,0)*m(1,2) - m(0,2)*m(1,0)) * invDet;
    out(2,2) = (m(0,0)*m(1,1) - m(0,1)*m(1,0)) * invDet;

    return true;
}

void Mat4::ScaleUniform(float x) {
    Scale(x, x, x);
}

void Mat4::Scale(float x, float y, float z) {
    _data[0] *= x;
    _data[1] *= x;
    _data[2] *= x;
    _data[4] *= y;
    _data[5] *= y;
    _data[6] *= y;
    _data[8] *= z;    
    _data[9] *= z;    
    _data[10] *= z;
}

// Derivation inspired by:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/orthographic-projection-matrix
//
// Important to note: this generates a z value that is more negative the closer
// to the camera it is. This is to match GL's default depth-testing.
Mat4 Mat4::Ortho(float width, float aspect, float zNear, float zFar) {
    Mat4 m;
    m(0,0) = 2.f / width;
    m(1,1) = 2.f * aspect / width;
    m(2,2) = -2.f / (zFar - zNear);
    m(2,3) = -(zFar + zNear) / (zFar - zNear);
    return m;
}

Mat4 Mat4::Ortho(float left, float right, float top, float bottom, float zNear, float zFar) {
    assert(right - left != 0.f);
    assert(top - bottom != 0.f);
    assert(zFar - zNear != 0.f);
    Mat4 m;
    m(0,0) = 2.f / (right - left);
    m(1,1) = 2.f / (top - bottom);
    m(2,2) = -2.f / (zFar - zNear);

    m(0,3) = -(right + left) / (right - left);
    m(1,3) = -(top + bottom) / (top - bottom);
    m(2,3) = -(zFar + zNear) / (zFar - zNear);
    return m;
}

Mat4 Mat4::LookAt(Vec3 const& eye, Vec3 const& at, Vec3 const& up) {
    Mat3 r;
    r.SetCol(2, (at - eye).GetNormalized());   
    r.SetCol(0, Vec3::Cross(r.GetCol(2), up).GetNormalized());
    r.SetCol(1, Vec3::Cross(r.GetCol(0), r.GetCol(2)).GetNormalized());
    r.SetCol(2, -r.GetCol(2));

    Vec3 t = r.MultiplyTranspose(-eye);
    Mat4 result;
    result.SetTopLeftMat3(r);
    result.SetTranslation(t);
    return result;  
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
    ymax = znear * tanf(0.5f * fovyRadians);
    // ymin = -ymax;
    // xmin = -ymax * aspectRatio;
    xmax = ymax * aspectRatio;
    return Frustrum(-xmax, xmax, -ymax, ymax, znear, zfar);
}
