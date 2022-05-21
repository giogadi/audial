#include "component.h"

#include <unordered_map>

#include "imgui/imgui.h"

#include "transform_manager.h"

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

void Component::DrawImGui() {
    ImGui::Text("(No properties)");
}

void TransformComponent::ConnectComponents(Entity& e, GameManager& g) {
    _mgr = g._transformManager;
    _mgr->AddTransform(this, &e);
}
void TransformComponent::Destroy() {
    _mgr->RemoveTransform(this);
}