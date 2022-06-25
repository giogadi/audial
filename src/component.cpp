#include "component.h"

#include <unordered_map>
#include <fstream>

#include "imgui/imgui.h"
#include "cereal/types/string.hpp"
#include "cereal/archives/xml.hpp"

#include "boost/property_tree/xml_parser.hpp"

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

void Entity::Save(ptree& pt) const {
    pt.put("name", _name);
    ptree& compsPt = pt.add_child("components", ptree());
    for (const auto& c_s : _components) {
        auto const& component = c_s->_c;
        ptree& compPt = compsPt.add_child("component", ptree());
        compPt.put("component_type", ComponentTypeToString(component->Type()));
        component->Save(compPt);
    }
}

void Entity::Load(ptree const& pt) {
    _name = pt.get<std::string>("name");
    for (auto const& item : pt.get_child("components")) {
        ptree const& compPt = item.second;
        std::string typeName = compPt.get<std::string>("component_type");
        ComponentType compType = StringToComponentType(typeName.c_str());
        std::shared_ptr<Component> newComp = TryAddComponentOfType(compType).lock();
        newComp->Load(compPt);
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

void EntityManager::Save(ptree& pt) const {
    ptree entitiesPt;
    this->ForEveryActiveAndInactiveEntity([&](EntityId id) {
        Entity const* e = this->GetEntity(id);
        ptree ePt;
        ePt.put("entity_active", this->IsActive(id));
        e->Save(ePt);
        entitiesPt.add_child("entity", ePt);
    });
    pt.add_child("entities", entitiesPt);
}

void EntityManager::Load(ptree const& pt) {
    for (auto const& item : pt.get_child("entities")) {
        ptree const& entityPt = item.second;
        bool active = entityPt.get<bool>("entity_active");
        EntityId id = this->AddEntity(active);
        Entity* entity = this->GetEntity(id);
        entity->Load(entityPt);
    }
}

template<typename Archive>
void save(Archive& ar, EntityManager const& eMgr) {
    int numEntities = 0;
    eMgr.ForEveryActiveAndInactiveEntity([&numEntities](EntityId id) {
        ++numEntities;
    });

    ar(cereal::make_nvp("num_entities", numEntities));

    eMgr.ForEveryActiveAndInactiveEntity([&ar, &eMgr](EntityId id) {
        Entity const* e = eMgr.GetEntity(id);
        ar(cereal::make_nvp("active", eMgr.IsActive(id)));
        ar(cereal::make_nvp("entity", *e));
    });
}

template<typename Archive>
void load(Archive& ar, EntityManager& eMgr) {
    int numEntities = 0;
    ar(numEntities);
    for (int i = 0; i < numEntities; ++i) {
        bool active;
        ar(active);
        EntityId id = eMgr.AddEntity(active);
        Entity* entity = eMgr.GetEntity(id);
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

// bool LoadEntities(char const* filename, bool dieOnConnectFailure, EntityManager& e, GameManager& g) {
//     std::ifstream inFile(filename);
//     if (!inFile.is_open()) {
//         std::cout << "Couldn't open file " << filename << " for loading." << std::endl;
//         return false;
//     }
//     cereal::XMLInputArchive archive(inFile);
//     archive(e);
//     e.ConnectComponents(g, dieOnConnectFailure);
//     return true;
// }

// bool SaveEntities(char const* filename, EntityManager const& entities) {
//     std::ofstream outFile(filename);
//     if (!outFile.is_open()) {
//         std::cout << "Couldn't open file " << filename << " for saving. Not saving." << std::endl;
//         return false;
//     }
//     cereal::XMLOutputArchive archive(outFile);
//     archive(CEREAL_NVP(entities));
//     return true;
// }

bool LoadEntities(char const* filename, bool dieOnConnectFailure, EntityManager& e, GameManager& g) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for loading." << std::endl;
        return false;
    }
    ptree pt;
    boost::property_tree::read_xml(inFile, pt);
    e.Load(pt);
    e.ConnectComponents(g, dieOnConnectFailure);
    return true;
}

bool SaveEntities(char const* filename, EntityManager const& entities) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for saving. Not saving." << std::endl;
        return false;
    }
    ptree pt;
    entities.Save(pt);
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(filename, pt.add_child("entities", pt), std::locale(), settings);
    return true;
}

void TransformComponent::Save(ptree& pt) const {
    _transform.Save(pt.add_child("mat4", ptree()));
}
void TransformComponent::Load(ptree const& pt) {
    _transform.Load(pt.get_child("mat4"));
}

void VelocityComponent::Save(ptree& pt) const {
    _linear.Save(pt.add_child("linear", ptree()));
    pt.put<float>("angularY", _angularY);
}
void VelocityComponent::Load(ptree const& pt) {
    _linear.Load(pt.get_child("linear"));
    _angularY = pt.get<float>("angularY");
}