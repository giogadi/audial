#pragma once

#include "property.h"
#include "serial.h"

bool ImGuiNodeStart(char const* label);
void ImGuiNodePop();

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

template<typename ObjectType>
struct PropertyMultiImGuiFunctor {
    ObjectType** objects;
    size_t numObjects;
    template<typename MemberPtrType>
    void Leaf(Property<MemberPtrType> property) {
        ObjectType& firstObject = *objects[0];
        auto& value = firstObject.*(property.pMember);
        bool changed = PropImGui(property.name, value);
        if (changed) {
            for (int objectIx = 0; objectIx < numObjects; ++objectIx) {
                ObjectType& o = *objects[objectIx];
                o.*(property.pMember) = value;
            }
        }        
    }
    template<typename MemberPtrType>
    void Node(Property<MemberPtrType> property) {
        if (ImGuiNodeStart(property.name)) {
            // THIS IS HORRIBLE
            auto childObjects = new decltype(GetPointerType(property.pMember))*[numObjects];
            for (int ii = 0; ii < numObjects; ++ii) {
                childObjects[ii] = &(objects[ii]->*(property.pMember));
            }
            PropertyMultiImGuiFunctor<decltype(GetPointerType(property.pMember))> fn;
            fn.objects = childObjects;
            fn.numObjects = numObjects;
            PropSerial<decltype(fn), decltype(GetPointerType(property.pMember))>::Serialize(fn);
    
            delete[] childObjects;
            ImGuiNodePop();
        } 
    }
};
template<typename ObjectType>
PropertyMultiImGuiFunctor<ObjectType> PropertyMultiImGuiFunctor_Make(ObjectType** objects, size_t numObjects) {
    PropertyMultiImGuiFunctor<ObjectType> fn;
    fn.objects = objects;
    fn.numObjects = numObjects;
    return fn;
}
