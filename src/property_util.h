#pragma once

#include <memory>

#include "property.h"
#include "serial.h"

bool ImGuiNodeStart(char const* label);
void ImGuiNodePop();

// https://godbolt.org/z/K3sG8fz63
class BumpAllocatorInternal {
public:

    static inline void* _buffer = nullptr;
    static inline size_t _bufferSize = 0;
    static inline size_t _spaceLeft = 0;
    static inline void* _front = nullptr;

    static void Init() {
        if (_buffer != nullptr) {
            free(_buffer);
        }
        _bufferSize = 1024 * 1024; 
        _spaceLeft = _bufferSize;
        _buffer = malloc(_bufferSize);
        _front = _buffer;
    }
    static void Cleanup() {
        if (_buffer != nullptr) {
            free(_buffer);
            _buffer = nullptr;
            _bufferSize = 0;
            _spaceLeft = 0;
            _front = nullptr;
        }
    }
    static void* allocate(size_t alignment, size_t size) {
        void* prev = _front;
        void* result = std::align(alignment, size, _front, _spaceLeft);
        if (result) {            
            _front = (char*)_front + size;
            _spaceLeft -= size;

            // printf("Allocation! %p to %p\n", prev, _front);
            return result;
        }
        return nullptr;        
    }

    static size_t GetOffset() {
        return (size_t)_front - (size_t)_buffer;
    }
    static void ReturnToOffset(size_t offset) {
        size_t currentOffset = GetOffset();
        if (offset == currentOffset) {
            return;
        }
        // printf("Returning: %p to %p\n", currentOffset, offset);
        if (offset >= currentOffset) {
            printf("HOWDY! current: %zu, requested: %zu\n", currentOffset, offset);
        }
        assert(offset < currentOffset);
        size_t change = currentOffset - offset;
        _spaceLeft += change;
        _front = (char*)_front - change;
    }
};

template<typename T>
class BumpAllocator {
public:
    typedef T value_type;
    BumpAllocator() noexcept {}

    template <class U> BumpAllocator (const BumpAllocator<U>&) noexcept {}
    T* allocate (size_t n) { 
        return reinterpret_cast<T*>(BumpAllocatorInternal::allocate(alignof(T), n * sizeof(T)));
    }
    void deallocate (T* p, std::size_t n) {}
};

template<typename Serializer, typename ObjectType>
struct PropSerial;

// Used to get the underlying type of a pointer-to-member
template <class C, typename T>
T GetPointerType(T C::*v);

template<typename ObjectType>
struct PropertySaveFunctor {
    ObjectType const* object; 
    serial::Ptree pt;
    template<typename MemberPtrType>
    void Leaf(Property<MemberPtrType> property) {
        PropSave(pt, property.name, (*object).*(property.pMember)); 
    }
    template<typename MemberPtrType>
    void Node(Property<MemberPtrType> property) {
        serial::Ptree childPt = pt.AddChild(property.name);
        PropertySaveFunctor<decltype(GetPointerType(property.pMember))> fn;
        fn.object = &((*object).*(property.pMember));
        fn.pt = childPt; 
        PropSerial<decltype(fn), decltype(GetPointerType(property.pMember))>::Serialize(fn);
    }
};
template<typename ObjectType>
PropertySaveFunctor<ObjectType> PropertySaveFunctor_Make(ObjectType const& object, serial::Ptree pt) {
    PropertySaveFunctor<ObjectType> fn;
    fn.object = &object;
    fn.pt = pt;
    return fn;
}

template<typename ObjectType>
struct PropertyLoadFunctor {
    ObjectType* object;
    serial::Ptree pt;
    template<typename MemberPtrType>
    void Leaf(Property<MemberPtrType> property) {
        PropLoad(pt, property.name, (*object).*(property.pMember));
    }
    template<typename MemberPtrType>
    void Node(Property<MemberPtrType> property) {
        serial::Ptree childPt = pt.TryGetChild(property.name);
        if (!childPt.IsValid()) {
            return;
        }
        PropertyLoadFunctor<decltype(GetPointerType(property.pMember))> fn;
        fn.object = &((*object).*(property.pMember));
        fn.pt = childPt;
        PropSerial<decltype(fn), decltype(GetPointerType(property.pMember))>::Serialize(fn);
    }
};
template<typename ObjectType>
PropertyLoadFunctor<ObjectType> PropertyLoadFunctor_Make(ObjectType& object, serial::Ptree pt) {
    PropertyLoadFunctor<ObjectType> fn;
    fn.object = &object;
    fn.pt = pt;
    return fn;
}

template<typename ObjectType>
struct PropertyImGuiFunctor {
    ObjectType* object;
    template<typename MemberPtrType>
    void Leaf(Property<MemberPtrType> property) {
        PropImGui(property.name, ((*object).*(property.pMember)));
    }
    template<typename MemberPtrType>
    void Node(Property<MemberPtrType> property) {
        if (ImGuiNodeStart(property.name)) {
            PropertyImGuiFunctor<decltype(GetPointerType(property.pMember))> fn;
            fn.object = &((*object).*(property.pMember));
            PropSerial<decltype(fn), decltype(GetPointerType(property.pMember))>::Serialize(fn);

            ImGuiNodePop();
        }
    }
};
template<typename ObjectType>
PropertyImGuiFunctor<ObjectType> PropertyImGuiFunctor_Make(ObjectType& object) {
    PropertyImGuiFunctor<ObjectType> fn;
    fn.object = &object;
    return fn;
}

template<typename ObjectType, typename BaseObjectType>
struct PropertyMultiImGuiFunctor {
    BaseObjectType** objects;
    size_t numObjects;
    template<typename MemberPtrType>
    void Leaf(Property<MemberPtrType> property) {
        ObjectType& firstObject = static_cast<ObjectType&>(*objects[0]);
        auto& value = firstObject.*(property.pMember);
        bool changed = PropImGui(property.name, value);
        if (changed) {
            for (int objectIx = 0; objectIx < numObjects; ++objectIx) {
                ObjectType& o = static_cast<ObjectType&>(*objects[objectIx]);
                o.*(property.pMember) = value;
            }
        }        
    }
    template<typename MemberPtrType>
    void Node(Property<MemberPtrType> property) {
        if (ImGuiNodeStart(property.name)) {
            size_t bumpOffset = BumpAllocatorInternal::GetOffset();

            BumpAllocator<decltype(GetPointerType(property.pMember))*> alloc;
            auto* childObjects = alloc.allocate(numObjects);
            for (int ii = 0; ii < numObjects; ++ii) {
                ObjectType& object = static_cast<ObjectType&>(*objects[ii]);
                childObjects[ii] = &(object.*(property.pMember));
            }
            PropertyMultiImGuiFunctor<decltype(GetPointerType(property.pMember)), decltype(GetPointerType(property.pMember))> fn;
            fn.objects = childObjects;
            fn.numObjects = numObjects;
            PropSerial<decltype(fn), decltype(GetPointerType(property.pMember))>::Serialize(fn);
    
            BumpAllocatorInternal::ReturnToOffset(bumpOffset);
            ImGuiNodePop();
        } 
    }
};
template<typename ObjectType, typename BaseObjectType>
PropertyMultiImGuiFunctor<ObjectType, BaseObjectType> PropertyMultiImGuiFunctor_Make(BaseObjectType** objects, size_t numObjects) {
    PropertyMultiImGuiFunctor<ObjectType, BaseObjectType> fn;
    fn.objects = objects;
    fn.numObjects = numObjects;
    return fn;
}
