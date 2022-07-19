#pragma once

#include "version_id.h"

// THIS ONE IS PROBABLY BROKEN ON REMOVE!!!
class VersionIdListInternal;
class VersionIdList {
public:
    VersionIdList(int itemSizeInBytes, int maxNumItems);
    ~VersionIdList();

    // DO NOT STORE THE POINTER!!
    VersionId AddItem(void** pOutItem);

    // Returns nullptr if the given id no longer points to a valid item
    void* GetItem(VersionId id);

    // Returns true if successful; false if this id wasn't in pool (i.e., may
    // have already been deleted)
    bool RemoveItem(VersionId id);

    // THESE ARE INTENDED FOR ITERATING THROUGH DENSE ARRAY OF ITEMS. DO NOT
    // STORE POINTERS AND DO NOT ADD/REMOVE ITEMS WHILE ITERATING.
    int GetCount() const { return _count; }
    void* GetItemAtIndex(int dataIndex);

private:
    char* _pData = nullptr;
    int32_t* _pSparseIndices = nullptr;
    int _itemSizeInBytes = -1;
    int _maxNumItems = -1;
    int _count = -1;

    int32_t* _pVersions = nullptr;
    int32_t* _pDataIndices = nullptr;
    VersionIdListInternal* _pInternal = nullptr;
};