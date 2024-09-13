#include "rng.h"

#include <random>

namespace rng {

namespace {
    State gRng;
}

void Seed(State& rng, unsigned int seed) {
    rng.s.seed(seed);
}

int GetInt(State& rng, int minVal, int maxVal) {
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(rng.s);
}

float GetFloat(State& rng, float minVal, float maxVal) {
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(rng.s);
}

void SeedGlobal(unsigned int seed) {
    Seed(gRng, seed);
}
int GetIntGlobal(int minVal, int maxVal) {
    return GetInt(gRng, minVal, maxVal);
}
float GetFloatGlobal(float minVal, float maxVal) {
    return GetFloat(gRng, minVal, maxVal);
}

}  // end namespace rng
