#pragma once

#include "matrix.h"
#include "quaternion.h"
#include "serial.h"

struct Transform {

public:    
    Vec3 Pos() const;
    void SetPos(Vec3 const& p);

    Quaternion const& Quat() const { return _q; }
    void SetQuat(Quaternion const& q);

    Vec3 const& Scale() const { return _scale; }
    void SetScale(Vec3 const& v) { _scale = v; }

    Mat4 const& Mat4NoScale() const;
    Mat4 Mat4Scale() const;

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    
private:
    void MaybeUpdateRotMat() const;
    
    // Stores reference position and cached rotation matrix (no scale!)
    mutable Mat4 _mat;

    // Reference rotation
    Quaternion _q;

    // Reference scale
    Vec3 _scale = Vec3(1.f, 1.f, 1.f);

    mutable bool _rotMatDirty = true;
};
