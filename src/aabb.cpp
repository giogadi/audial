#include "aabb.h"

#include "rng.h"

Vec3 Aabb::SampleRandom() const {
    Vec3 r;
    r._x = rng::GetFloatGlobal(_min._x, _max._x);
    r._y = rng::GetFloatGlobal(_min._y, _max._y);
    r._z = rng::GetFloatGlobal(_min._z, _max._z);
    return r;
}
