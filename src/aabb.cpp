#include "aabb.h"

#include "rng.h"

Vec3 Aabb::SampleRandom() const {
    Vec3 r;
    r._x = rng::GetFloat(_min._x, _max._x);
    r._y = rng::GetFloat(_min._y, _max._y);
    r._z = rng::GetFloat(_min._z, _max._z);
    return r;
}
