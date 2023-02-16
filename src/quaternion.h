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

    // ASSUMES AXIS IS A UNIT VECTOR
    void SetFromAngleAxis(float angleRad, Vec3 const& axis);

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
};
