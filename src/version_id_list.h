#pragma once

#include <vector>
#include <algorithm>

#include "version_id.h"

template <typename T>
class TVersionIdList {
public:
    TVersionIdList() {}
    TVersionIdList(int32_t reserveNumItems) {
        _denseData.reserve(reserveNumItems);
        _sparseIndices.reserve(reserveNumItems);
        _versions.reserve(reserveNumItems);
        _denseIndices.reserve(reserveNumItems);
    }

    VersionId AddItem(T** pOutItem) {
        if (_freeSparseIndices.empty()) {
            int32_t index = _denseData.size();
            VersionId newId(index, 0);
            _versions.push_back(0);
            _denseIndices.push_back(index);
            _denseData.emplace_back();
            _sparseIndices.push_back(index);
            if (pOutItem != nullptr) {
                *pOutItem = &(_denseData.back());
            }
            return newId;
        } else {
            int freeSparseIndex = _freeSparseIndices.back();
            assert(freeSparseIndex >= 0);
            assert(freeSparseIndex < _versions.size());
            assert(freeSparseIndex < _denseIndices.size());
            _freeSparseIndices.pop_back();
            VersionId newId(freeSparseIndex, _versions.at(freeSparseIndex));
            _denseData.emplace_back();
            _denseIndices.at(freeSparseIndex) = _denseData.size() - 1;
            _sparseIndices.push_back(freeSparseIndex);
            if (pOutItem != nullptr) {
                *pOutItem = &(_denseData.back());
            }
            return newId;
        }
    }

    T* GetItem(VersionId id) {
        assert(id.IsValid());
        int32_t sparseIndex = id.GetIndex();
        assert(sparseIndex < _versions.size());
        assert(sparseIndex < _denseIndices.size());
        if (id.GetVersion() == _versions.at(sparseIndex)) {
            int32_t denseIndex = _denseIndices.at(sparseIndex);
            assert(denseIndex >= 0);
            assert(denseIndex < _denseData.size());
            return &(_denseData.at(denseIndex));
        }
        return nullptr;
    }

    bool RemoveItem(VersionId id) {
        assert(id.IsValid());
        int32_t sparseIndex = id.GetIndex();
        assert(sparseIndex < _versions.size());
        assert(sparseIndex < _denseIndices.size());
        if (_versions.at(sparseIndex) == id.GetVersion()) {
            assert(_denseData.size() > 0);
            _versions.at(sparseIndex) += 1;
            int32_t denseHoleIndex = _denseIndices.at(sparseIndex);
            assert(denseHoleIndex >= 0);
            _denseIndices.at(sparseIndex) = -1;
            _freeSparseIndices.push_back(sparseIndex);
            if (_denseData.size() > 1) {
                // in dense set, move the last item into this new hole
                std::swap(_denseData.back(), _denseData.at(denseHoleIndex));

                int32_t sparseIndex = _sparseIndices.back();
                _denseIndices.at(sparseIndex) = denseHoleIndex;
                std::swap(_sparseIndices.back(), _sparseIndices.at(denseHoleIndex));
            }
            // item to remove is guaranteed to be last
            _denseData.pop_back();
            _sparseIndices.pop_back();
            return true;
        }
        return false;
    }

    int GetCount() const { return _denseData.size(); }
    
    // TODO: CAN'T REMEMBER is this guaranteed to be non-null?
    T* GetItemAtIndex(int32_t denseIndex) {
        return &(_denseData.at(denseIndex));
    }
private:
    // Take dense index
    std::vector<T> _denseData;
    std::vector<int32_t> _sparseIndices;

    // Take sparse index
    std::vector<int32_t> _versions;
    std::vector<int32_t> _denseIndices;

    std::vector<int32_t> _freeSparseIndices;
};
