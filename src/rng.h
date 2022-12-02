#pragma once

namespace rng {
    void Seed(unsigned int seed);

    int GetInt(int minVal, int maxVal);
    float GetFloat(float minVal, float maxVal);
}
