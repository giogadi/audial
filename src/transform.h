#pragma once

#include "matrix.h"
#include "quaternion.h"
#include "serial.h"

struct Transform {

public:    
    Vec3 Pos() const;
    void SetPos(Vec3 const& p);
    void SetPosX(float x);
    void SetPosY(float y);
    void SetPosZ(float z);

    void Translate(Vec3 const& t);
    
    // functions to make refactors easier
    Vec3 GetPos() const { return Pos(); }
    void SetTranslation(Vec3 const& p) { SetPos(p); }

    Quaternion const& Quat() const { return _q; }
    void SetQuat(Quaternion const& q);

    Vec3 const& Scale() const { return _scale; }
    void SetScale(Vec3 const& v) { _scale = v; }
    void ApplyScale(Vec3 const& v);

    Mat4 const& Mat4NoScale() const;
    // TODO maybe replace this with one that populates an out-param to not temp RVO
    Mat4 Mat4Scale() const;   
    void SetFromMat4(Mat4 const& mat4);

    // Returns normalized vectors
    Vec3 GetXAxis() const;
    Vec3 GetYAxis() const;
    Vec3 GetZAxis() const;

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    
private:
    void MaybeUpdateRotMat() const;
    
    // Stores reference position and cached rotation matrix (no scale!!). This
    // needs to be updated whenever the quaternion changes.
    mutable Mat4 _mat;

    // Reference rotation
    Quaternion _q;

    // Reference scale
    Vec3 _scale = Vec3(1.f, 1.f, 1.f);

    mutable bool _rotMatDirty = true;
};
