#pragma once

#include <string>

#include "rng.h"

namespace string_util {

inline std::string GenerateRandomText(int numChars) {
    std::string text;
    text.reserve(numChars);
    for (int i = 0; i < numChars; ++i) {
        int randCharIx = rng::GetInt(0, 25);
        char randChar = 'a' + (char) randCharIx;
        text.push_back(randChar);
    }
    return text;
}

}
