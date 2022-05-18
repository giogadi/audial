#include "entity_loader.h"

#include <fstream>

#include "cereal/types/string.hpp"
#include "cereal/archives/xml.hpp"
#include "serialize.h"

template<typename Archive>
void save(Archive& ar, Entity const& e) {
    ar(CEREAL_NVP(e._name));
    ar(cereal::make_nvp("num_components", e._components.size()));
    for (auto const& pComp : e._components) {
        ComponentType compType = pComp->Type();
        std::string compTypeName = ComponentTypeToString(compType);
        ar(cereal::make_nvp("component_type", compTypeName));
        switch (compType) {
#           define X(name) \
            case ComponentType::name: { \
                name##Component* c = dynamic_cast<name##Component*>(pComp.get()); \
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
        std::string compTypeName;
        ar(compTypeName);
        ComponentType compType = StringToComponentType(compTypeName.c_str());
        std::unique_ptr<Component> pComp;
        switch (compType) {
#           define X(name) \
            case ComponentType::name: { \
                auto c = std::make_unique<name##Component>(); \
                ar(*c); \
                pComp = std::move(c); \
                break; \
            }
            M_COMPONENT_TYPES
#           undef X
            case ComponentType::NumTypes:
                assert(false);
                break;
        }
        e._components.push_back(std::move(pComp));
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
        Entity* entity = e.AddEntity();
        ar(*entity);
    }
}

void LoadEntities(char const* filename, EntityManager& e, GameManager& g) {    
    std::ifstream inFile(filename);
    cereal::XMLInputArchive archive(inFile);
    archive(e);
    e.ConnectComponents(g);
}

void SaveEntities(char const* filename, EntityManager& entities) {
    std::ofstream outFile(filename);
    cereal::XMLOutputArchive archive(outFile);
    archive(CEREAL_NVP(entities));
}