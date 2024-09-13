#pragma once

#include <string>
#include <charconv>
#include <string_view>
#include <optional>
#include <cstdint>
#include <cassert>

#include "rng.h"

namespace string_util {

inline std::string GenerateRandomText(int numChars) {
    std::string text;
    text.reserve(numChars);
    for (int i = 0; i < numChars; ++i) {
        int randCharIx = rng::GetIntGlobal(0, 25);
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
inline bool MaybeStof(std::string const& str, float& output) {    
    try {
        output = std::stof(str);
        return true;
    } catch (std::exception&) {
        return false;
    }
}
inline bool MaybeStof(std::string_view const str, float& output) {
    // WTF, doesn't work yet with string_view???
    // float x;
    // auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.length(), x);
    // assert(ec == std::errc());
    // return x;
    return MaybeStof(std::string(str), output);
}
inline std::optional<float> MaybeStof(std::string const& str) {
    float x;
    if (MaybeStof(str, x)) {
        return x;
    } else {
        return std::nullopt;
    }
}
inline float StofOrDie(std::string_view const str) {
    float x;
    bool success = MaybeStof(std::string(str), x);
    assert(success);
    return x;
}
inline bool MaybeStoi64(std::string_view const str, int64_t& output) {
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.length(), output);
    return ec == std::errc();
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

inline void ToUpper(std::string& x) {
    for (char& c : x) {
        c = toupper(c);
    }
}

inline bool Contains(std::string_view const str, std::string_view const substr) {
    auto result = str.find(substr);
    return result != std::string_view::npos;
    /*if (str.empty() || substr.empty() || substr.length() > str.length()) {
        return false;
    }
    int const endIx = str.length() - substr.length() + 1;
    for (int strIx = 0; strIx < endIx; ++strIx) {
        bool match = true;
        for (int substrIx = 0; substrIx < substr.length(); ++substrIx) {
            if (str[strIx + substrIx] != substr[substrIx]) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;*/
}

}  // namespace string_util
