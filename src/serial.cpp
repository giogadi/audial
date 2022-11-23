#include "serial.h"

#include <fstream>

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"

using boost::property_tree::ptree;

namespace serial {

namespace {
    ptree* GetInternal(void* p) {
        return (ptree*)p;
    }
}

Ptree Ptree::AddChild(char const* name) {
    assert(IsValid());
    ptree& internalChildPt = GetInternal(_internal)->add_child(name, ptree());
    Ptree childPt;
    childPt._internal = (void*)(&internalChildPt);
    return childPt;
}

Ptree Ptree::GetChild(char const* name) {
    assert(IsValid());
    ptree& internalChildPt = GetInternal(_internal)->get_child(name);
    Ptree childPt;
    childPt._internal = (void*)(&internalChildPt);
    return childPt;
}

Ptree Ptree::TryGetChild(char const* name) {
    assert(IsValid());
    boost::optional<ptree&> maybeChildPtInternal = GetInternal(_internal)->get_child_optional(name);
    Ptree childPt;
    if (maybeChildPtInternal.has_value()) {
        childPt._internal = (void*)(&maybeChildPtInternal.value());
    }
    return childPt;
}

void Ptree::PutString(char const* name, char const* v) {
    assert(IsValid());
    GetInternal(_internal)->add<std::string>(name, v);
}
std::string Ptree::GetString(char const* name) {
    return GetInternal(_internal)->get<std::string>(name);
}
bool Ptree::TryGetString(char const* name, std::string* v) {
    assert(IsValid());
    auto const maybe_string = GetInternal(_internal)->get_optional<std::string>(name);
    if (maybe_string.has_value()) {
        *v = maybe_string.value();
        return true;
    }
    return false;
}

void Ptree::PutBool(char const* name, bool b) {
    assert(IsValid());
    GetInternal(_internal)->add(name, b);
}
bool Ptree::GetBool(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->get<bool>(name);
}
bool Ptree::TryGetBool(char const* name, bool* v) {
    assert(IsValid());
    auto const maybe_val = GetInternal(_internal)->get_optional<bool>(name);
    if (maybe_val.has_value()) {
        *v = maybe_val.value();
        return true;
    }
    return false;
}

void Ptree::PutInt(char const* name, int v) {
    assert(IsValid());
    GetInternal(_internal)->add(name, v);
}
int Ptree::GetInt(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->get<int>(name);
}
bool Ptree::TryGetInt(char const* name, int* v) {
    assert(IsValid());
    auto const maybe_val = GetInternal(_internal)->get_optional<int>(name);
    if (maybe_val.has_value()) {
        *v = maybe_val.value();
        return true;
    }
    return false;
}

void Ptree::PutLong(char const* name, long v) {
    assert(IsValid());
    GetInternal(_internal)->add(name, v);
}
long Ptree::GetLong(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->get<long>(name);
}
bool Ptree::TryGetLong(char const* name, long* v) {
    assert(IsValid());
    auto const maybe_val = GetInternal(_internal)->get_optional<long>(name);
    if (maybe_val.has_value()) {
        *v = maybe_val.value();
        return true;
    }
    return false;
}

void Ptree::PutFloat(char const* name, float v) {
    assert(IsValid());
    GetInternal(_internal)->add(name, v);
}
float Ptree::GetFloat(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->get<float>(name);
}
bool Ptree::TryGetFloat(char const* name, float* v) {
    assert(IsValid());
    auto const maybe_val = GetInternal(_internal)->get_optional<float>(name);
    if (maybe_val.has_value()) {
        *v = maybe_val.value();
        return true;
    }
    return false;
}

void Ptree::PutDouble(char const* name, double v) {
    assert(IsValid());
    GetInternal(_internal)->add(name, v);
}
double Ptree::GetDouble(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->get<double>(name);
}
bool Ptree::TryGetDouble(char const* name, double* v) {
    assert(IsValid());
    auto const maybe_val = GetInternal(_internal)->get_optional<double>(name);
    if (maybe_val.has_value()) {
        *v = maybe_val.value();
        return true;
    }
    return false;
}

std::string Ptree::GetStringValue() {
    assert(IsValid());
    return GetInternal(_internal)->get_value<std::string>();
}

int Ptree::GetIntValue() {
    assert(IsValid());
    return GetInternal(_internal)->get_value<int>();
}

NameTreePair* Ptree::GetChildren(int* numChildren) {
    assert(IsValid());
    NameTreePair* children = new NameTreePair[GetInternal(_internal)->size()];
    // void* rawMemory = malloc(_internal->size() * sizeof(NameTreePair));
    // NameTreePair* children = static_cast<NameTreePair*>(rawMemory);
    int index = 0;
    ptree* internal = GetInternal(_internal);
    for (auto& item : *internal) {
        NameTreePair& c = children[index];
        c._name = item.first.c_str();
        c._pt._internal = (void*)(&item.second);
        ++index;
    }
    *numChildren = index;
    return children;
}

Ptree::Ptree(Ptree const& other) {
    _internal = other._internal;
}

Ptree& Ptree::operator=(Ptree const& other) {
    _internal = other._internal;
    return *this;
}

Ptree Ptree::MakeNew() {
    Ptree pt;
    pt._internal = (void*)(new ptree);
    return pt;
}

void Ptree::DeleteData() {
    delete GetInternal(_internal);
    _internal = nullptr;
}

bool Ptree::WriteToFile(char const* filename) {
    assert(_internal != nullptr);
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        printf("Couldn't open file %s for saving. Not saving.\n", filename);
        return false;
    }
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(outFile, *GetInternal(_internal), settings);
    return true;
}

bool Ptree::LoadFromFile(char const* filename) {
    assert(_internal != nullptr);
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        printf("Couldn't open file %s for loading.\n", filename);
        return false;
    }
    boost::property_tree::read_xml(inFile, *GetInternal(_internal));
    return true;
}

} // namespace serial
