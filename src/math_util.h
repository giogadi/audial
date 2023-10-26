#pragma once

#include "matrix.h"

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

inline double Clamp(double x, double low, double high) {
    if (x < low) {
        return low;
    }
    if (x > high) {
        return high;
    }
    return x;
}

inline int Clamp(int x, int low, int high) {
    if (x < low) {
        return low;
    }
    if (x > high) {
        return high;
    }
    return x;
}

template<typename T>
inline T ClampAbs(T x, T maxAbsValue) {
    return Clamp(x, -maxAbsValue, maxAbsValue);
}

inline float SmoothStep(float x) {
    if (x <= 0) { return 0.f; }
    if (x >= 1.f) { return 1.f; }
    float x2 = x*x;
    return 3*x2 - 2*x2*x;
}

// Maps [0..1] to [0..1..0]
inline float Triangle(float x) {
    return -std::abs(2.f*(x - 0.5f)) + 1.f;
}

inline float SmoothUpAndDown(float x) {
    if (x < 0.5f) {
        return SmoothStep(2*x);
    }
    return 1.f-SmoothStep(2*(x - 0.5f));
}

inline Vec3 Vec3Lerp(float x, Vec3 const& a, Vec3 const& b) {
    Vec3 res = b - a;
    res *= x;
    res += a;
    return res;
}

}
