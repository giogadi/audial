#include "serial.h"

#include <fstream>

#include "boost/property_tree/xml_parser.hpp"

namespace serial {

Ptree Ptree::AddChild(char const* name) {
    assert(IsValid());
    ptree& internalChildPt = _internal->add_child(name, ptree());
    Ptree childPt;
    childPt._internal = &internalChildPt;
    return childPt;
}

Ptree Ptree::GetChild(char const* name) {
    assert(IsValid());
    ptree& internalChildPt = _internal->get_child(name);
    Ptree childPt;
    childPt._internal = &internalChildPt;
    return childPt;
}

void Ptree::PutString(char const* name, char const* v) {
    assert(IsValid());
    _internal->add<std::string>(name, v);
}

std::string Ptree::GetString(char const* name) {
    assert(IsValid());
    return _internal->get<std::string>(name);
}

void Ptree::PutBool(char const* name, bool b) {
    assert(IsValid());
    _internal->add(name, b);
}

void Ptree::PutInt(char const* name, int v) {
    assert(IsValid());
    _internal->add(name, v);
}

void Ptree::PutLong(char const* name, long v) {
    assert(IsValid());
    _internal->add(name, v);
}

void Ptree::PutFloat(char const* name, float v) {
    assert(IsValid());
    _internal->add(name, v);
}

void Ptree::PutDouble(char const* name, double v) {
    assert(IsValid());
    _internal->add(name, v);
}

Ptree::Ptree(Ptree const& other) {
    if (_owned) {
        delete _internal;
    }
    _owned = false;
    _internal = other._internal;
}

Ptree& Ptree::operator=(Ptree const& other) {
    if (_owned) {
        delete _internal;
    }
    _owned = false;
    _internal = other._internal;
    return *this;
}

Ptree::~Ptree() {
    if (_owned) {
        delete _internal;
    }
}

Ptree Ptree::MakeNew() {
    Ptree pt;
    pt._owned = true;
    pt._internal = new ptree;
    return pt;
}

bool Ptree::WriteToFile(char const* filename) {
    assert(_internal != nullptr);
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        printf("Couldn't open file %s for saving. Not saving.\n", filename);
        return false;
    }
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(outFile, *_internal, settings);
    return true;
}

} // namespace serial