#include "rng.h"

#include <random>

namespace rng {

namespace {
    std::mt19937 _rng;
}

void Seed(unsigned int seed) {
    _rng.seed(seed);
}

int GetInt(int minVal, int maxVal) {
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(_rng);
}

}  // end namespace rng
