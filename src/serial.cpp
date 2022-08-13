#include "serial.h"

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
    _internal->put<std::string>(name, v);
}

std::string Ptree::GetString(char const* name) {
    assert(IsValid());
    return _internal->get<std::string>(name);
}

Ptree::Ptree(Ptree const& other) {
    _owned = false;
    _internal = other._internal;
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

} // namespace serial