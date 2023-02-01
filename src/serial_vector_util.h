#pragma once

#include <vector>

#include "serial.h"

namespace serial {

template <typename T>
inline void SaveVectorInChildNode(serial::Ptree pt, char const* childName, char const* itemName, std::vector<T> const& v) {
    serial::Ptree vecPt = pt.AddChild(childName);
    for (T const& t : v) {
        SaveInNewChildOf(vecPt, itemName, t);
    }
}

template<>
inline void SaveVectorInChildNode<int>(serial::Ptree pt, char const* childName, char const* itemName, std::vector<int> const& v) {
    serial::Ptree vecPt = pt.AddChild(childName);
    for (int const x : v) {
        serial::Ptree itemPt = vecPt.AddChild(itemName);
        itemPt.PutIntValue(x);
    }
}

template<>
inline void SaveVectorInChildNode<bool>(serial::Ptree pt, char const* childName, char const* itemName, std::vector<bool> const& v) {
    serial::Ptree vecPt = pt.AddChild(childName);
    for (bool const x : v) {
        serial::Ptree itemPt = vecPt.AddChild(itemName);
        itemPt.PutBoolValue(x);
    }
}

template<>
inline void SaveVectorInChildNode<float>(serial::Ptree pt, char const* childName, char const* itemName, std::vector<float> const& v) {
    serial::Ptree vecPt = pt.AddChild(childName);
    for (float const x : v) {
        serial::Ptree itemPt = vecPt.AddChild(itemName);
        itemPt.PutFloatValue(x);
    }
}

template<>
inline void SaveVectorInChildNode<double>(serial::Ptree pt, char const* childName, char const* itemName, std::vector<double> const& v) {
    serial::Ptree vecPt = pt.AddChild(childName);
    for (double const x : v) {
        serial::Ptree itemPt = vecPt.AddChild(itemName);
        itemPt.PutDoubleValue(x);
    }
}

template<>
inline void SaveVectorInChildNode<std::string>(serial::Ptree pt, char const* childName, char const* itemName, std::vector<std::string> const& v) {
    serial::Ptree vecPt = pt.AddChild(childName);
    for (std::string const& x : v) {
        serial::Ptree itemPt = vecPt.AddChild(itemName);
        itemPt.PutStringValue(x.c_str());
    }
}

template <typename T>
inline bool LoadVectorFromChildNode(serial::Ptree pt, char const* childName, std::vector<T>& v) {
    serial::Ptree vecPt = pt.TryGetChild(childName);
    if (!vecPt.IsValid()) {
        return false;
    }
    int numChildren;
    serial::NameTreePair* children = vecPt.GetChildren(&numChildren);
    v.clear();
    v.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        v.emplace_back();
        v.back().Load(children[i]._pt);
    }
    delete[] children;
    return true;
}

template <>
inline bool LoadVectorFromChildNode<int>(serial::Ptree pt, char const* childName, std::vector<int>& v) {
    serial::Ptree vecPt = pt.TryGetChild(childName);
    if (!vecPt.IsValid()) {
        return false;
    }
    int numChildren;
    serial::NameTreePair* children = vecPt.GetChildren(&numChildren);
    v.clear();
    v.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        int x = children[i]._pt.GetIntValue();
        v.push_back(x);
    }
    delete[] children;
    return true;
}

template <>
inline bool LoadVectorFromChildNode<std::string>(serial::Ptree pt, char const* childName, std::vector<std::string>& v) {
    serial::Ptree vecPt = pt.TryGetChild(childName);
    if (!vecPt.IsValid()) {
        return false;
    }
    int numChildren;
    serial::NameTreePair* children = vecPt.GetChildren(&numChildren);
    v.clear();
    v.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        std::string x = children[i]._pt.GetStringValue();
        v.push_back(std::move(x));
    }
    delete[] children;
    return true;
}

template <>
inline bool LoadVectorFromChildNode<bool>(serial::Ptree pt, char const* childName, std::vector<bool>& v) {
    serial::Ptree vecPt = pt.TryGetChild(childName);
    if (!vecPt.IsValid()) {
        return false;
    }
    int numChildren;
    serial::NameTreePair* children = vecPt.GetChildren(&numChildren);
    v.clear();
    v.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        bool x = children[i]._pt.GetBoolValue();
        v.push_back(x);
    }
    delete[] children;
    return true;
}

template <>
inline bool LoadVectorFromChildNode<float>(serial::Ptree pt, char const* childName, std::vector<float>& v) {
    serial::Ptree vecPt = pt.TryGetChild(childName);
    if (!vecPt.IsValid()) {
        return false;
    }
    int numChildren;
    serial::NameTreePair* children = vecPt.GetChildren(&numChildren);
    v.clear();
    v.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        float x = children[i]._pt.GetFloatValue();
        v.push_back(x);
    }
    delete[] children;
    return true;
}

template <>
inline bool LoadVectorFromChildNode<double>(serial::Ptree pt, char const* childName, std::vector<double>& v) {
    serial::Ptree vecPt = pt.TryGetChild(childName);
    if (!vecPt.IsValid()) {
        return false;
    }
    int numChildren;
    serial::NameTreePair* children = vecPt.GetChildren(&numChildren);
    v.clear();
    v.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        double x = children[i]._pt.GetDoubleValue();
        v.push_back(x);
    }
    delete[] children;
    return true;
}

}
