#include "component.h"

#include <unordered_map>
#include <fstream>

#include "imgui/imgui.h"

#include "boost/property_tree/xml_parser.hpp"

// Here we go
#include "renderer.h"
#include "components/rigid_body.h"
#include "components/player_controller.h"
#include "components/beep_on_hit.h"
#include "components/sequencer.h"
#include "components/player_orbit_controller.h"
#include "components/camera_controller.h"
#include "components/hit_counter.h"
#include "components/orbitable.h"
#include "components/events_on_hit.h"
#include "components/activator.h"
#include "components/damage.h"
#include "components/on_destroy_event.h"

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

void Entity::ConnectComponentsOrDie(EntityId id, GameManager& g) {
    for (auto& compAndStatus : _components) {
        if (compAndStatus->_active) {
            assert(compAndStatus->_c->ConnectComponents(id, *this, g));
        }
    }
}

void Entity::ConnectComponentsOrDeactivate(
    EntityId id, GameManager& g, std::vector<ComponentType>* failures) {
    if (_components.empty()) {
        return;
    }
    std::vector<bool> active(_components.size());
    for (int i = 0; i < _components.size(); ++i) {
        active[i] = _components[i]->_active;
    }
    bool allActiveSucceeded = false;
    for (int roundIx = 0; !allActiveSucceeded && roundIx < _components.size() + 1; ++roundIx) {
        // Clone components into a parallel vector, delete the original, start over.
        ptree entityData;
        this->Save(entityData);
        _components.clear();
        _componentTypeMap.clear();
        this->Load(entityData);

        // std::stringstream entityData;
        // SaveEntity(entityData, *this);
        // _components.clear();
        // _componentTypeMap.clear();
        // LoadEntity(entityData, *this);

        allActiveSucceeded = true;
        for (int i = 0; i < _components.size(); ++i) {
            if (active[i]) {
                bool success = _components[i]->_c->ConnectComponents(id, *this, g);
                if (!success) {
                    active[i] = false;
                    allActiveSucceeded = false;
                }
            }
        }
    }
    assert(allActiveSucceeded);
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

void EntityManager::ConnectComponents(GameManager& g, bool dieOnConnectFailure) {
    ForEveryActiveEntity([&](EntityId id) {
        Entity* e = this->GetEntity(id);
        if (dieOnConnectFailure) {
            e->ConnectComponentsOrDie(id, g);
        } else {
            e->ConnectComponentsOrDeactivate(id, g, /*failures=*/nullptr);
        }
    });
}

void EntityManager::Save(ptree& pt) const {
    this->ForEveryActiveAndInactiveEntity([&](EntityId id) {
        Entity const* e = this->GetEntity(id);
        ptree ePt;
        ePt.put("entity_active", this->IsActive(id));
        e->Save(ePt);
        pt.add_child("entity", ePt);
    });
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
    {
        ptree& entitiesPt = pt.add_child("entities", ptree());
        entities.Save(entitiesPt);
    }
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(outFile, pt, settings);
    return true;
}

bool SaveEntity(char const* filename, Entity const& e) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for saving. Not saving." << std::endl;
        return false;
    }
    ptree pt;
    {
        ptree& entityPt = pt.add_child("entity", ptree());
        e.Save(entityPt);
    }
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(outFile, pt, settings);
    return true;
}

bool LoadEntity(char const* filename, Entity& e) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for loading." << std::endl;
        return false;
    }
    ptree pt;
    boost::property_tree::read_xml(inFile, pt);
    e.Load(pt.get_child("entity"));
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