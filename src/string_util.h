#pragma once

#include <string>
#include <charconv>
#include <string_view>

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

inline int StoiOrDie(std::string_view str) {
    int x;
    auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.length(), x);
    assert(ec == std::errc());
    return x;
}
inline float StofOrDie(std::string_view str) {
    // WTF, doesn't work yet???
    // float x;
    // auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.length(), x);
    // assert(ec == std::errc());
    // return x;
    std::string ownedStr(str);
    return std::stof(ownedStr);
}

}
