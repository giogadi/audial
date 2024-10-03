#include "rng.h"

#include <climits>
#include <cfloat>

#if !(UINT32_MAX && UINT64_MAX && INT_MAX <= UINT32_MAX && \
    (32 - FLT_MANT_DIG) >= 7 && (64 - DBL_MANT_DIG) >= 7)
# error rng.h: platform not supported
#endif

namespace rng {

namespace {
    State gRng;

    uint32_t xorshift32(State& state)
    {
        uint32_t x = state.s;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state.s = x;
    }

    // https://github.com/camel-cdr/cauldron/blob/main/cauldron/random.h
    float GetFloat01(uint32_t r) {
        return (r >> (32 - FLT_MANT_DIG)) *
            (1.0f / (UINT32_C(1) << FLT_MANT_DIG));
    }
}

void Seed(State& rng, uint32_t seed) {
    rng.s = seed;
}

int GetInt(State& rng, int minVal, int maxVal) {
    uint32_t r = xorshift32(rng);
    int x = minVal + (int32_t)(r % (uint32_t(maxVal - minVal) + 1));
    return x;
}

float GetFloat(State& rng, float minVal, float maxVal) {
    uint32_t r = xorshift32(rng);

    float f01 = GetFloat01(r);
    float x = minVal + (f01 * (maxVal - minVal));
    return x;
}

void SeedGlobal(uint32_t seed) {
    Seed(gRng, seed);
}
int GetIntGlobal(int minVal, int maxVal) {
    return GetInt(gRng, minVal, maxVal);
}
float GetFloatGlobal(float minVal, float maxVal) {
    return GetFloat(gRng, minVal, maxVal);
}

}  // end namespace rng
