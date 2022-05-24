#include "component.h"

#include <unordered_map>
#include <fstream>

#include "imgui/imgui.h"
#include "cereal/types/string.hpp"
#include "cereal/archives/xml.hpp"

#include "serialize.h"
#include "audio_serialize.h"

namespace {

std::unordered_map<std::string, ComponentType> const gStringToComponentType = {
#   define X(name) {#name, ComponentType::name},
    M_COMPONENT_TYPES
#   undef X
};

}  // end namespace

char const* gComponentTypeStrings[] = {
#   define X(a) #a,
    M_COMPONENT_TYPES
#   undef X
};

char const* ComponentTypeToString(ComponentType c) {
    return gComponentTypeStrings[static_cast<int>(c)];
}

ComponentType StringToComponentType(char const* c) {
    return gStringToComponentType.at(c);
}

bool Component::DrawImGui() {
    ImGui::Text("(No properties)");
    return false;
}

std::weak_ptr<Component> Entity::TryAddComponentOfType(ComponentType c) {
    switch (c) {
#       define X(name) \
        case ComponentType::name: { \
            return this->TryAddComponent<name##Component>(); \
        }
        M_COMPONENT_TYPES
#       undef X
        case ComponentType::NumTypes:
            assert(false);
            break;
    }
}

template<typename Archive>
void save(Archive& ar, Entity const& e) {
    ar(CEREAL_NVP(e._name));
    int const numComponents = e.GetNumComponents();
    ar(cereal::make_nvp("num_components", numComponents));
    for (int compIx = 0; compIx < numComponents; ++compIx) {
        Component const& comp = e.GetComponent(compIx);
        ComponentType compType = comp.Type();
        ar(cereal::make_nvp("component_type", compType));
        switch (compType) {
#           define X(name) \
            case ComponentType::name: { \
                name##Component const* c = dynamic_cast<name##Component const*>(&comp); \
                assert(c != nullptr); \
                ar(cereal::make_nvp(#name, *c)); \
                break; \
            }
            M_COMPONENT_TYPES
#           undef X    
            case ComponentType::NumTypes:
                assert(false);
                break;
        }
    }
}

template<typename Archive>
void load(Archive& ar, Entity& e) {
    ar(e._name);
    int numComponents = 0;
    ar(numComponents);
    for (int i = 0; i < numComponents; ++i) {
        ComponentType compType;
        ar(compType);
        switch (compType) {
#           define X(name) \
            case ComponentType::name: { \
                auto c = e.AddComponentOrDie<name##Component>().lock(); \
                ar(*c); \
                break; \
            }
            M_COMPONENT_TYPES
#           undef X
            case ComponentType::NumTypes:
                assert(false);
                break;
        }
    }
}

template<typename Archive>
void save(Archive& ar, EntityManager const& e) {
    ar(cereal::make_nvp("num_entities", e._entities.size()));
    for (auto const& pEntity : e._entities) {
        ar(cereal::make_nvp("entity", *pEntity));
    }
}

template<typename Archive>
void load(Archive& ar, EntityManager& e) {
    int numEntities = 0;
    ar(numEntities);
    for (int i = 0; i < numEntities; ++i) {
        Entity* entity = e.AddEntity().lock().get();
        ar(*entity);
    }
}

void SaveEntity(std::ostream& output, Entity const& e) {
    cereal::XMLOutputArchive archive(output);
    archive(CEREAL_NVP(e));
}

void LoadEntity(std::istream& input, Entity& e) {
    cereal::XMLInputArchive archive(input);
    archive(e);
}

bool LoadEntities(char const* filename, bool dieOnConnectFailure, EntityManager& e, GameManager& g) {    
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for loading." << std::endl;
        return false;
    }
    cereal::XMLInputArchive archive(inFile);
    archive(e);
    e.ConnectComponents(g, dieOnConnectFailure);
    return true;
}

void SaveEntities(char const* filename, EntityManager& entities) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for saving. Not saving." << std::endl;
        return;
    }
    cereal::XMLOutputArchive archive(outFile);
    archive(CEREAL_NVP(entities));
}