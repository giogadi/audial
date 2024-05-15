#pragma once

#include "property.h"
#include "serial.h"

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
    void operator()(Property<MemberPtrType> property) {
        PropImGui(property.name, &((*object).*(property.pMember)));
    }
};

template<typename ObjectType>
struct PropertyMultiImGuiFunctor {
    ObjectType* objects;
    size_t numObjects;
    template<typename MemberPtrType>
    void operator()(Property<MemberPtrType> property) {
        decltype(GetPointerType(property.pMember)) value;
        ObjectType& firstObject = objects[0];
        value = firstObject.*(property.pMember);
        bool changed = PropImGui(property.name, &value);
        if (changed) {
            for (int objectIx = 0; objectIx < numObjects; ++objectIx) {
                ObjectType& o = objects[objectIx];
                o.*(property.pMember) = value;
            }
        }
    }
};
