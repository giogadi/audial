#pragma once

namespace math_util {
    inline float Clamp(float x, float low, float high) {
        if (x < low) {
            return low;
        }
        if (x > high) {
            return high;
        }
        return x;
    }
}