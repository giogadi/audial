#pragma once

#include "matrix.h"
#include "serial.h"

struct Quaternion {
    Vec4 _v; 
  
    Quaternion();  
    Quaternion(Vec4 v) { _v = v; }

    Quaternion Conjugate() const;
    float Magnitude() const;
    Quaternion Inverse() const;

    Quaternion operator*(Quaternion const& rhs) const;

    void GetRotMat(Mat3& mat) const;
    void GetRotMat(Mat4& mat) const;

    // ASSUMES INPUT MAT IS ORTHONORMALIZED. ADD A CHECK FOR THIS HERE!
    void SetFromRotMat(Mat3 const& mat);

    // ASSUMES AXIS IS A UNIT VECTOR
    void SetFromAngleAxis(float angleRad, Vec3 const& axis);

    // rotation order: Y, then X, then Z
    Vec3 EulerAngles() const;
    void SetFromEulerAngles(Vec3 const& e);

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
};
