#include "entity.h"

#include "boost/property_tree/xml_parser.hpp"

// Here we go
#include "components/transform.h"
#include "components/model_instance.h"
#include "components/light.h"
#include "components/camera.h"
#include "components/velocity.h"
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

void Entity::Update(float dt) {
    for (auto& c : _components) {
        if (c->_active) {
            c->_c->Update(dt);
        }
    }
}

void Entity::EditModeUpdate(float dt) {
    for (auto& c : _components) {
        if (c->_active) {
            c->_c->EditModeUpdate(dt);
        }
    }
}

void Entity::Destroy() {
    for (auto& c : _components) {
        // TODO: do I need to consider component._active here?
        c->_c->Destroy();
    }
    ResetWithoutComponentDestroy();
}

void Entity::ResetWithoutComponentDestroy() {
    _components.clear();
    _componentTypeMap.clear();
}

void Entity::RemoveComponent(int index) {
    ComponentType compType = _components[index]->_c->Type();
    for (auto item = _componentTypeMap.begin(); item != _componentTypeMap.end(); ++item) {
        if (item->second->_c->Type() == compType) {
            _componentTypeMap.erase(item);
            break;
        }
    }
    // _components[index]->_c->Destroy();
    _components.erase(_components.begin() + index);
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

bool Entity::Save(char const* filename) const {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for saving. Not saving." << std::endl;
        return false;
    }
    ptree pt;
    {
        ptree& entityPt = pt.add_child("entity", ptree());
        this->Save(entityPt);
    }
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(outFile, pt, settings);
    return true;
}

bool Entity::Load(char const* filename) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for loading." << std::endl;
        return false;
    }
    ptree pt;
    boost::property_tree::read_xml(inFile, pt);
    this->Load(pt.get_child("entity"));
    return true;
}