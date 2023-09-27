#pragma once

#include <cstdint>
#include <istream>

#include "serial.h"

struct EditorId {
    int64_t _id = -1;
    bool IsValid() const { return _id >= 0; }
    bool operator==(EditorId const& rhs) const { return _id == rhs._id; }
    bool operator<=(EditorId const& rhs) const { return _id <= rhs._id; }
    inline void Save(serial::Ptree pt) const { pt.PutInt64("id", _id); }
    inline void Load(serial::Ptree pt) { _id = pt.GetInt64("id"); }
    bool SetFromString(std::string const& s);
    std::string ToString() const;
};

inline std::istream& operator>>(std::istream& in, EditorId& id) {
    return in >> id._id;
}