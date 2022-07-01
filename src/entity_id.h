#pragma once

typedef int32_t EntityIndex;
typedef int32_t EntityVersion;

class EntityId {
public:
    constexpr EntityId(EntityIndex index, EntityVersion version) :
        _id(((int64_t)index << 32) | ((int64_t) version)) {}

    EntityIndex GetIndex() const {
        return _id >> 32;
    }
    EntityVersion GetVersion() const {
        return (int32_t)_id;
    }
    static constexpr EntityId InvalidId() {
        return EntityId(-1, 0);
    }
    bool IsValid() const {
        return GetIndex() >= 0;
    }
    bool operator==(EntityId rhs) const {
        return _id == rhs._id;
    }
    bool operator!=(EntityId rhs) const {
        return !(*this == rhs);
    }
private:
    int64_t _id;
};