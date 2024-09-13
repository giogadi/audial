#pragma once

#include <random>

namespace rng {
    struct State {
        std::mt19937 s;
    };
    void Seed(State& rng, unsigned int seed);
    int GetInt(State& rng, int minVal, int maxVal);
    float GetFloat(State& rng, float minVal, float maxVal);

    void SeedGlobal(unsigned int seed);
    int GetIntGlobal(int minVal, int maxVal);
    float GetFloatGlobal(float minVal, float maxVal);
}
