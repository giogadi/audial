#include "version_id_list_simple.h"

#include <vector>

class VersionIdListInternal {
public:
    std::vector<int> _freeIndices;
};

VersionIdList::VersionIdList(int itemSizeInBytes, int maxNumItems)
    : _itemSizeInBytes(itemSizeInBytes)
    , _maxNumItems(maxNumItems)
    , _count(0) {
    // TODO: maybe allocate this all at once in a big block?
    _pData = new char[maxNumItems * itemSizeInBytes];
    _pSparseIndices = new int32_t[maxNumItems];
    memset(_pSparseIndices, -1, maxNumItems);
    _pVersions = new int32_t[maxNumItems];
    memset(_pVersions, -1, maxNumItems);
    _pDataIndices = new int32_t[maxNumItems];
    memset(_pDataIndices, -1, maxNumItems);
    _pInternal = new VersionIdListInternal;
}

VersionIdList::~VersionIdList() {
    delete[] _pData;
    delete[] _pSparseIndices;
    delete[] _pVersions;
    delete[] _pDataIndices;
    delete _pInternal;
}

void* VersionIdList::GetItem(VersionId id) {
    assert(id.IsValid());
    int32_t index = id.GetIndex();
    assert(index < _maxNumItems);
    if (id.GetVersion() == _pVersions[index]) {
        int dataIndex = _pDataIndices[index];
        assert(dataIndex >= 0);
        assert(dataIndex < _maxNumItems);
        return _pData + (_itemSizeInBytes * dataIndex);
    }
    return nullptr;
}

VersionId VersionIdList::AddItem(void** pOutItem) {
    if (_pInternal->_freeIndices.empty()) {
        int index = _count;
        VersionId newId(index, 0);
        _pVersions[index] = 0;
        _pDataIndices[index] = index;
        _pSparseIndices[index] = index;
        ++_count;
        if (pOutItem != nullptr) {
            *pOutItem = _pData + (_itemSizeInBytes * index);
        }
        return newId;
    } else {
        int freeIndex = _pInternal->_freeIndices.back();
        assert(freeIndex < _maxNumItems);
        _pInternal->_freeIndices.pop_back();
        VersionId newId(freeIndex, _pVersions[freeIndex]);
        _pDataIndices[freeIndex] = _count;
        _pSparseIndices[_count] = freeIndex;
        if (pOutItem != nullptr) {
            *pOutItem = _pData + (_itemSizeInBytes * _count);
        }
        ++_count;
        return newId;
    }
}

bool VersionIdList::RemoveItem(VersionId id) {
    assert(id.IsValid());
    int index = id.GetIndex();
    assert(index < _maxNumItems);
    if (_pVersions[index] == id.GetVersion()) {
        _pVersions[index] += 1;
        int holeIndex = _pDataIndices[index];
        assert(holeIndex >= 0);
        _pDataIndices[index] = -1;
        _pInternal->_freeIndices.push_back(index);
        --_count;
        if (_count > 0) {
            // in dense set, move the last item into this new hole
            char* pLastItem = _pData + (_itemSizeInBytes * _count);
            char* pHole = _pData + (_itemSizeInBytes * holeIndex);
            memcpy(pHole, pLastItem, _itemSizeInBytes);
            //_pDataIndices[_count] = holeIndex;
            int32_t lastItemSparseIndex = _pSparseIndices[_count];
            _pDataIndices[lastItemSparseIndex] = holeIndex;
            _pSparseIndices[_count] = -1;
            _pSparseIndices[holeIndex] = lastItemSparseIndex;
        }
        return true;
    }
    return false;
}

void* VersionIdList::GetItemAtIndex(int dataIndex) {
    return _pData + (_itemSizeInBytes * dataIndex);
}