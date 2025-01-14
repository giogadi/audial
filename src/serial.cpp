#include "serial.h"

#include <fstream>
#include <cassert>

#include "tinyxml2/tinyxml2.h"

using namespace tinyxml2;

namespace serial {

namespace {
    XMLElement* GetInternal(void* p) {
        return (XMLElement*)p;
    }
    XMLDocument* GetDoc(void* p) {
        return (XMLDocument*)p;
    }

    int const kBinaryVersion = 14;
}

Ptree Ptree::AddChild(char const* name) {
    assert(IsValid());
    XMLElement *internalChildPt = GetInternal(_internal)->InsertNewChildElement(name);
    Ptree childPt;
    childPt._internal = (void*)(internalChildPt);
    childPt._version = _version;
    return childPt;
}

Ptree Ptree::GetChild(char const* name) {
    assert(IsValid());
    XMLElement *internalChildPt = GetInternal(_internal)->FirstChildElement(name);
    Ptree childPt;
    childPt._internal = (void*)(internalChildPt);
    childPt._version = _version;
    return childPt;
}

Ptree Ptree::TryGetChild(char const* name) {
    return GetChild(name);
}

void Ptree::PutString(char const* name, char const* v) {
    assert(IsValid());
    GetInternal(_internal)->InsertNewChildElement(name)->SetText(v);
}
std::string Ptree::GetString(char const* name) {
    XMLElement *element = GetInternal(_internal)->FirstChildElement(name);
    char const *text = element->GetText();
    if (text) {
        return std::string(text);
    } else {
        return std::string();
    }
}
bool Ptree::TryGetString(char const* name, std::string* v) {
    assert(IsValid());
    XMLElement *e = GetInternal(_internal)->FirstChildElement(name);
    if (e) {
        if (e->GetText()) {
            *v = std::string(e->GetText());
        } else {
            *v = std::string();
        }
        return true;
    }
    return false; 
}

void Ptree::PutBool(char const* name, bool b) {
    assert(IsValid());
    GetInternal(_internal)->InsertNewChildElement(name)->SetText(b);
}
bool Ptree::GetBool(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->FirstChildElement(name)->BoolText();
}
bool Ptree::TryGetBool(char const* name, bool* v) {
    assert(IsValid());
    XMLElement *e = GetInternal(_internal)->FirstChildElement(name);
    if (e) {
        *v = e->BoolText();
        return true;
    }
    return false; 
}

void Ptree::PutChar(char const* name, char v) {
    assert(IsValid());
    char buf[2] = { v, '\0' };
    GetInternal(_internal)->InsertNewChildElement(name)->SetText(buf);
}
bool Ptree::TryGetChar(char const* name, char* v) {
    assert(IsValid());
    XMLElement *e = GetInternal(_internal)->FirstChildElement(name);
    if (e) {
        char const* text = e->GetText();
        *v = text[0];
        return true;
    } 
    return false;
}

void Ptree::PutInt(char const* name, int v) {
    assert(IsValid());
    GetInternal(_internal)->InsertNewChildElement(name)->SetText(v);;
}
int Ptree::GetInt(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->FirstChildElement(name)->IntText();
}
bool Ptree::TryGetInt(char const* name, int* v) {
    assert(IsValid());
    if (XMLElement *e = GetInternal(_internal)->FirstChildElement(name)) {
        *v = e->IntText();
        return true;
    }
    return false; 
}

void Ptree::PutInt64(char const* name, int64_t v) {
    assert(IsValid());
    GetInternal(_internal)->InsertNewChildElement(name)->SetText(v);
}
int64_t Ptree::GetInt64(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->FirstChildElement(name)->Int64Text();
}
bool Ptree::TryGetInt64(char const* name, int64_t* v) {
    assert(IsValid());
    if (XMLElement *e = GetInternal(_internal)->FirstChildElement(name)) {
        *v = e->Int64Text();
        return true;
    }
    return false;
}

void Ptree::PutFloat(char const* name, float v) {
    assert(IsValid());
    GetInternal(_internal)->InsertNewChildElement(name)->SetText(v);
}
float Ptree::GetFloat(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->FirstChildElement(name)->FloatText();
}
bool Ptree::TryGetFloat(char const* name, float* v) {
    assert(IsValid());
    if (XMLElement *e = GetInternal(_internal)->FirstChildElement(name)) {
        *v = e->FloatText();
        return true;
    }
    return false; 
}

void Ptree::PutDouble(char const* name, double v) {
    assert(IsValid());
    GetInternal(_internal)->InsertNewChildElement(name)->SetText(v);
}
double Ptree::GetDouble(char const* name) {
    assert(IsValid());
    return GetInternal(_internal)->FirstChildElement(name)->DoubleText();
}
bool Ptree::TryGetDouble(char const* name, double* v) {
    assert(IsValid());
    if (XMLElement *e = GetInternal(_internal)->FirstChildElement(name)) {
        *v = e->DoubleText();
        return true;
    }
    return false; 
}

void Ptree::PutStringValue(char const* value) {
    assert(IsValid());
    GetInternal(_internal)->SetText(value);
}

void Ptree::PutIntValue(int value) {
    assert(IsValid());
    GetInternal(_internal)->SetText(value);
}

void Ptree::PutInt64Value(int64_t value) {
    assert(IsValid());
    GetInternal(_internal)->SetText(value);
}

void Ptree::PutBoolValue(bool value) {
    assert(IsValid());
    GetInternal(_internal)->SetText(value);
}

void Ptree::PutFloatValue(float value) {
    assert(IsValid());
    GetInternal(_internal)->SetText(value);
}

void Ptree::PutDoubleValue(double value) {
    assert(IsValid());
    GetInternal(_internal)->SetText(value);
}

std::string Ptree::GetStringValue() {
    assert(IsValid());
    return GetInternal(_internal)->GetText();
}

int Ptree::GetIntValue() {
    assert(IsValid());
    return GetInternal(_internal)->IntText();
}

int64_t Ptree::GetInt64Value() {
    assert(IsValid());
    return GetInternal(_internal)->Int64Text();
}

bool Ptree::GetBoolValue() {
    assert(IsValid());
    return GetInternal(_internal)->BoolText();
}

float Ptree::GetFloatValue() {
    assert(IsValid());
    return GetInternal(_internal)->FloatText();
}

double Ptree::GetDoubleValue() {
    assert(IsValid());
    return GetInternal(_internal)->DoubleText();
}

NameTreePair* Ptree::GetChildren(int* numChildren) {
    assert(IsValid());
    *numChildren = GetInternal(_internal)->ChildElementCount();
    NameTreePair* children = new NameTreePair[*numChildren];
    int index = 0;
    XMLElement *child = GetInternal(_internal)->FirstChildElement();
    while (child != nullptr) {
        NameTreePair &c = children[index];
        c._name = child->Name();
        c._pt._internal = (void*)(child);
        c._pt._version = _version;
        child = child->NextSiblingElement();
        ++index;
    }
    assert(index == *numChildren);
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
    pt._internal = (void*)(new XMLDocument);
    pt._version = kBinaryVersion;
    return pt;
}

void Ptree::DeleteData() {
    delete GetDoc(_internal);
    _internal = nullptr;
}

bool Ptree::WriteToFile(char const* filename) {
    XMLDocument versionedDoc;
    versionedDoc.InsertEndChild(versionedDoc.NewElement("root"));
    versionedDoc.RootElement()->InsertNewChildElement("version")->SetText(_version);
    XMLNode *treeCopy = GetDoc(_internal)->RootElement()->DeepClone(&versionedDoc);
    versionedDoc.RootElement()->InsertEndChild(treeCopy);

    versionedDoc.SaveFile(filename);
    return true;
}

bool Ptree::LoadFromFile(char const* filename) {
    assert(_internal != nullptr);
    GetDoc(_internal)->LoadFile(filename);
    _version = GetDoc(_internal)->FirstChildElement()->FirstChildElement("version")->IntText();
    return true;
}

} // namespace serial
