#pragma once

#include <serial.h>

// TODO: This will only work on structs with "standard layout". Easiest way to do that is to stick with straight C, no virtual anything. UGH

typedef void (*PropLoadFn)(void* v, serial::Ptree pt);
typedef void (*PropSaveFn)(void const* v, serial::Ptree pt);
typedef bool (*PropImGuiFn)(char const* name, void* v);
struct PropertySpec {
    PropLoadFn loadFn;
    PropSaveFn saveFn;
    PropImGuiFn imguiFn; 
};

struct Property {
    char const* name;
    PropertySpec* spec; 
    size_t offset;
};

void LoadProperties(Property const* props, size_t numProperties, serial::Ptree pt, void* object);
void SaveProperties(Property const* props, size_t numProperties, serial::Ptree pt, void const* object);

extern PropertySpec const gPropertyIntSpec;
extern PropertySpec const gPropertyInt64Spec;
extern PropertySpec const gPropertyFloatSpec;
extern PropertySpec const gPropertyDoubleSpec;
extern PropertySpec const gPropertyCharSpec;
extern PropertySpec const gPropertyBoolSpec;
extern PropertySpec const gPropertyString32Spec;
extern PropertySpec const gPropertyString128Spec;

extern PropertySpec const gPropertyVec3Spec;
extern PropertySpec const gPropertyColor4Spec;
