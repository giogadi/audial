#pragma once

#include "serial.h"

namespace serial {

template <typename E>
inline void PutEnum(Ptree pt, char const* name, E const v) {
    char const* vStr = EnumToString(v);
    pt.PutString(name, vStr);
}

template <typename E>
inline bool TryGetEnum(Ptree pt, char const* name, E& v) {
    std::string vStr;
    bool found = pt.TryGetString(name, &vStr);
    if (!found) {
        return false;
    }
    StringToEnum(vStr.c_str(), v);
    return true;
}

}
