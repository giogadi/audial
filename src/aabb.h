#pragma once

#include "matrix.h"

struct Aabb {
    Vec3 _min;
    Vec3 _max;

    void Save(serial::Ptree pt) const {
        serial::SaveInNewChildOf(pt, "min", _min);
        serial::SaveInNewChildOf(pt, "max", _max);
    }
    void Load(serial::Ptree pt) {
        _min.Load(pt.GetChild("min"));
        _max.Load(pt.GetChild("max"));
    }

    static Aabb MakeCube(float half_width) {
        Aabb a;
        a._min = Vec3(-half_width, -half_width, -half_width);
        a._max = Vec3(half_width, half_width, half_width);
        return a;
    }
};