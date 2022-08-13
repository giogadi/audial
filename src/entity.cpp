#include "entity.h"

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
#include "components/waypoint_follow.h"
#include "components/recorder_balloon.h"

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

void Entity::EditDestroy() {
    for (auto& c : _components) {
        // TODO: do I need to consider component._active here?
        c->_c->EditDestroy();
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
    std::vector<bool> active(_components.size(), true);
    bool allActiveSucceeded = false;
    for (int roundIx = 0; !allActiveSucceeded && roundIx < _components.size() + 1; ++roundIx) {
        // Clone components into a parallel vector, delete the original, start over.        
        serial::Ptree entityData = serial::Ptree::MakeNew();
        this->Save(entityData);
        this->EditDestroy();
        this->Load(entityData);

        allActiveSucceeded = true;
        for (int i = 0; i < _components.size(); ++i) {
            _components[i]->_active = active[i];
            if (active[i]) {
                bool success = _components[i]->_c->ConnectComponents(id, *this, g);
                if (!success) {
                    _components[i]->_active = false;
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

void Entity::Save(serial::Ptree pt) const {
    pt.PutString("name", _name.c_str());
    serial::Ptree compsPt = pt.AddChild("components");
    for (auto const& c_s : _components) {
        auto const& component = c_s->_c;
        serial::Ptree compPt = compsPt.AddChild("component");
        compPt.PutString("component_type", ComponentTypeToString(component->Type()));
        component->Save(compPt);
    }
}

void Entity::Load(serial::Ptree pt) {
    _name = pt.GetString("name");
    serial::Ptree componentsPt = pt.GetChild("components");
    int numChildren = 0;
    serial::NameTreePair* children = componentsPt.GetChildren(&numChildren);
    for (int i = 0; i < numChildren; ++i) {
        serial::Ptree& compPt = children[i]._pt;
        std::string typeName = compPt.GetString("component_type");
        ComponentType compType = StringToComponentType(typeName.c_str());
        std::shared_ptr<Component> newComp = TryAddComponentOfType(compType).lock();
        newComp->Load(compPt);
    }
    // free(children);
    delete[] children;
}

bool Entity::Save(char const* filename) const {
    serial::Ptree pt = serial::Ptree::MakeNew();
    {
        serial::Ptree entityPt = pt.AddChild("entity");
        this->Save(entityPt);
    }
    return pt.WriteToFile(filename);
}

bool Entity::Load(char const* filename) {
    serial::Ptree pt = serial::Ptree::MakeNew();
    return pt.LoadFromFile(filename);
}