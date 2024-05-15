#pragma once

template<typename MemberPtrType>
struct Property {
    char const* name;
    MemberPtrType pMember;
};

template<typename MemberPtrType>
inline Property<MemberPtrType> MakeProperty(char const* name, MemberPtrType pMember) {
    Property<MemberPtrType> property;
    property.name = name;
    property.pMember = pMember;
    return property;
}
