#pragma once

#include <serial.h>

namespace imgui_util {

struct Char {
    constexpr Char() : c('a'), bufInternal {'a', '\0'} {}
    constexpr Char(char c) : c(c), bufInternal { c, '\0' } {}
    char c;
    char bufInternal[2];

    void Save(serial::Ptree pt, char const* name) const;
    void Load(serial::Ptree pt, char const* name);
    bool ImGui(char const* name);
};

}
