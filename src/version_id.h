#pragma once

#include <cstdint>

class VersionId {
public:
    constexpr VersionId() : _id(MakeInvalid()) {}

    constexpr VersionId(int32_t index, int32_t version) :
        _id(MakeId(index, version)) {}

    int32_t GetIndex() const {
        return _id >> 32;
    }
    int32_t GetVersion() const {
        return (int32_t)_id;
    }
    static constexpr VersionId InvalidId() {
        VersionId invalidId;
        return invalidId;
    }
    bool IsValid() const {
        return GetIndex() >= 0;
    }
    bool operator==(VersionId rhs) const {
        return _id == rhs._id;
    }
    bool operator!=(VersionId rhs) const {
        return !(*this == rhs);
    }

private:
    static constexpr int64_t MakeId(int32_t index, int32_t version) {
        return ((int64_t)index << 32) | ((int64_t) version);
    }
    static constexpr int64_t MakeInvalid() {
        return MakeId(-1, 0);
    }

    int64_t _id;
};