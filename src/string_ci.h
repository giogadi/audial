#pragma once

#include <map>
#include <string>
#include <string_view>
#include <cctype>

namespace string_ci {

struct LessChar {
    bool operator()(char lhs, char rhs) {
        return tolower(lhs) < tolower(rhs);
    }
};

struct LessStr {
    bool operator()(std::string const& lhs, std::string const& rhs);
};

template <typename T>
struct map {
    std::map<std::string, T, LessStr> Type;
};

bool Contains(std::string_view str, std::string_view substr);

}
