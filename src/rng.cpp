#include "rng.h"

#include <random>

namespace rng {

namespace {
    std::mt19937 gRng;
}

void Seed(unsigned int seed) {
    gRng.seed(seed);
}

int GetInt(int minVal, int maxVal) {
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(gRng);
}

float GetFloat(float minVal, float maxVal) {
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(gRng);
}

}  // end namespace rng
