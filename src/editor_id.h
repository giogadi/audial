#pragma once

#include <cstdint>
#include <istream>
#include <unordered_map>

#include "serial.h"

typedef std::unordered_map<int64_t, int64_t> EditorIdMap;

struct EditorId {
    int64_t _id = -1;
    EditorId() {}
    explicit EditorId(int64_t id) : _id(id) {}
    bool IsValid() const { return _id >= 0; }
    bool operator==(EditorId const& rhs) const { return _id == rhs._id; }
    bool operator<=(EditorId const& rhs) const { return _id <= rhs._id; }
    inline void Save(serial::Ptree pt) const { pt.PutInt64("id", _id); }
    void Load(serial::Ptree pt);
    bool SetFromString(std::string const& s);
    std::string ToString() const;
};

inline std::istream& operator>>(std::istream& in, EditorId& id) {
    return in >> id._id;
}


