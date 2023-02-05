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

inline bool MaybeStoi(std::string_view const str, int& output) {
    auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.length(), output);
    return ec == std::errc();
}
inline int StoiOrDie(std::string_view const str) {
    int x;
    bool success = MaybeStoi(str, x);
    assert(success);
    return x;
}
inline bool MaybeStof(std::string_view const str, float& output) {
    // WTF, doesn't work yet???
    // float x;
    // auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.length(), x);
    // assert(ec == std::errc());
    // return x;
    std::string ownedStr(str);
    try {
        output = std::stof(ownedStr);
        return true;
    } catch (std::exception& e) {
        return false;
    }
}
inline float StofOrDie(std::string_view const str) {
    float x;
    bool success = MaybeStof(str, x);
    assert(success);
    return x;
}

inline bool EqualsIgnoreCase(std::string_view const x, std::string_view const y) {
    int xn = x.length();
    int yn = y.length();
    if (xn != yn) {
        return false;
    }
    for (int i = 0; i < xn; ++i) {
        if (tolower(x[i]) != tolower(y[i])) {
            return false;
        }
    }
    return true;
}

inline void ToLower(std::string& x) {
    for (char& c : x) {
        c = tolower(c);
    }
}

}  // namespace string_util
