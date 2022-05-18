#include "component.h"

#include <unordered_map>

namespace {
char const* gComponentTypeStrings[] = {
#   define X(a) #a,
    M_COMPONENT_TYPES
#   undef X
};

std::unordered_map<std::string, ComponentType> const gStringToComponentType = {
#   define X(name) {#name, ComponentType::name},
    M_COMPONENT_TYPES
#   undef X
};
}  // end namespace

char const* ComponentTypeToString(ComponentType c) {
    return gComponentTypeStrings[static_cast<int>(c)];
}

ComponentType StringToComponentType(char const* c) {
    return gStringToComponentType.at(c);
}