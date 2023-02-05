#pragma once

#include "matrix.h"
#include "enums/Direction.h"

namespace camera_util {

inline Vec3 GetDefaultCameraOffset(Direction dir) {
    float constexpr kYOffset = 5.f;
    switch (dir) {
        case Direction::Center:
            return Vec3(0.f, kYOffset, 0.f);
        case Direction::Up:
            return Vec3(0.f, 5.f, 3.f);
        case Direction::Down:
            return Vec3(0.f, 5.f, -3.f);
        case Direction::Left:
            return Vec3(3.f, 5.f, 0.f);
        case Direction::Right:
            return Vec3(-3.f, 5.f, 0.f);
        case Direction::Count:
            assert(false);
            return Vec3();
    }
}

}  // namespace camera_util
