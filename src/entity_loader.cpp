#include "entity_loader.h"

#include <fstream>

#include "cereal/types/string.hpp"
#include "cereal/archives/xml.hpp"
#include "serialize.h"
#include "audio_serialize.h"

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
                auto c = e.AddComponent<name##Component>().lock(); \
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
        Entity* entity = e.AddEntity();
        ar(*entity);
    }
}

bool LoadEntities(char const* filename, EntityManager& e, GameManager& g) {    
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for loading." << std::endl;
        return false;
    }
    cereal::XMLInputArchive archive(inFile);
    archive(e);
    e.ConnectComponents(g);
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