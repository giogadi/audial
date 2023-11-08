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

    int const kBinaryVersion = 1;
}

Ptree Ptree::AddChild(char const* name) {
    assert(IsValid());
    ptree& internalChildPt = GetInternal(_internal)->add_child(name, ptree());
    Ptree childPt;
    childPt._internal = (void*)(&internalChildPt);
    childPt._version = _version;
    return childPt;
}

Ptree Ptree::GetChild(char const* name) {
    assert(IsValid());
    ptree& internalChildPt = GetInternal(_internal)->get_child(name);
    Ptree childPt;
    childPt._internal = (void*)(&internalChildPt);
    childPt._version = _version;
    return childPt;
}

Ptree Ptree::TryGetChild(char const* name) {
    assert(IsValid());
    boost::optional<ptree&> maybeChildPtInternal = GetInternal(_internal)->get_child_optional(name);
    Ptree childPt;
    if (maybeChildPtInternal.has_value()) {
        childPt._internal = (void*)(&maybeChildPtInternal.value());
        childPt._version = _version;
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

void Ptree::PutInt64(char const* name, int64_t v) {
    assert(IsValid());
    GetInternal(_internal)->add(name, v);
}
int64_t Ptree::GetInt64(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->get<int64_t>(name);
}
bool Ptree::TryGetInt64(char const* name, int64_t* v) {
    assert(IsValid());
    auto const maybe_val = GetInternal(_internal)->get_optional<int64_t>(name);
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

void Ptree::PutStringValue(char const* value) {
    assert(IsValid());
    GetInternal(_internal)->put_value<std::string>(value);
}

void Ptree::PutIntValue(int value) {
    assert(IsValid());
    GetInternal(_internal)->put_value<int>(value);
}

void Ptree::PutBoolValue(bool value) {
    assert(IsValid());
    GetInternal(_internal)->put_value<bool>(value);
}

void Ptree::PutFloatValue(float value) {
    assert(IsValid());
    GetInternal(_internal)->put_value<float>(value);
}

void Ptree::PutDoubleValue(double value) {
    assert(IsValid());
    GetInternal(_internal)->put_value<double>(value);
}

std::string Ptree::GetStringValue() {
    assert(IsValid());
    return GetInternal(_internal)->get_value<std::string>();
}

int Ptree::GetIntValue() {
    assert(IsValid());
    return GetInternal(_internal)->get_value<int>();
}

int64_t Ptree::GetInt64Value() {
    assert(IsValid());
    return GetInternal(_internal)->get_value<int64_t>();
}

bool Ptree::GetBoolValue() {
    assert(IsValid());
    return GetInternal(_internal)->get_value<bool>();
}

float Ptree::GetFloatValue() {
    assert(IsValid());
    return GetInternal(_internal)->get_value<float>();
}

double Ptree::GetDoubleValue() {
    assert(IsValid());
    return GetInternal(_internal)->get_value<double>();
}

NameTreePair* Ptree::GetChildren(int* numChildren) {
    assert(IsValid());
    NameTreePair* children = new NameTreePair[GetInternal(_internal)->size()];
    int index = 0;
    ptree* internal = GetInternal(_internal);
    for (auto& item : *internal) {
        NameTreePair& c = children[index];
        c._name = item.first.c_str();
        c._pt._internal = (void*)(&item.second);
        c._pt._version = _version;
        ++index;
    }
    *numChildren = index;
    return children;
}

Ptree::Ptree(Ptree const& other) {
    _internal = other._internal;
    _version = other._version;
}

Ptree& Ptree::operator=(Ptree const& other) {
    _internal = other._internal;
    _version = other._version;
    return *this;
}

Ptree Ptree::MakeNew() {
    Ptree pt;
    pt._internal = (void*)(new ptree);
    pt._version = kBinaryVersion;
    return pt;
}

void Ptree::DeleteData() {
    delete GetInternal(_internal);
    _internal = nullptr;
}

bool Ptree::WriteToFile(char const* filename) {
    assert(_internal != nullptr);
    Ptree versionedPt = MakeNew();
    Ptree rootPt = versionedPt.AddChild("root");
    rootPt.PutInt("version", kBinaryVersion);
    boost::property_tree::ptree* rootInternal = GetInternal(rootPt._internal);
    // Get single root of content tree
    auto& contentRoot = GetInternal(_internal)->front();
    rootInternal->push_back(contentRoot);
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        printf("Couldn't open file %s for saving. Not saving.\n", filename);
        return false;
    }
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(outFile, *GetInternal(versionedPt._internal), settings);
    versionedPt.DeleteData();
    return true;
}

bool Ptree::LoadFromFile(char const* filename) {
    assert(_internal != nullptr);
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        printf("Couldn't open file %s for loading.\n", filename);
        return false;
    }
    GetInternal(_internal)->clear();
    _version = 0;
    boost::property_tree::ptree versionedInternal;
    boost::property_tree::read_xml(inFile, versionedInternal);
    try {
        boost::property_tree::ptree& rootInternal = versionedInternal.get_child("root");
        boost::property_tree::ptree::const_iterator iter = rootInternal.begin();        
        if (iter->first == "version") {
            _version = iter->second.get_value<int>();
            ++iter;
        }
        GetInternal(_internal)->push_back(*iter);
    } catch (std::exception& e) {
        *GetInternal(_internal) = versionedInternal;
    }
    
    return true;
}

} // namespace serial
