#pragma once

#include <cstdint>

namespace rng {
    struct State {
        uint32_t s;
    };
    void Seed(State& rng, uint32_t seed);
    int GetInt(State& rng, int minVal, int maxVal);
    float GetFloat(State& rng, float minVal, float maxVal);

    void SeedGlobal(uint32_t seed);
    int GetIntGlobal(int minVal, int maxVal);
    float GetFloatGlobal(float minVal, float maxVal);
}

