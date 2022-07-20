#pragma once

#include <typeinfo>
#include <typeindex>
#include <unordered_map>

#include "entity_id.h"
#include "component.h"

class GameManager;

class Entity {
public:
    // Only finds active components.
    template <typename T>
    std::weak_ptr<T> FindComponentOfType() {
        auto iter = _componentTypeMap.find(std::type_index(typeid(T)));
        if (iter == _componentTypeMap.end()) {
            // std::cout << "Could not find component" << std::endl;
            return std::weak_ptr<T>();
        }
        ComponentAndStatus* compAndStatus = iter->second;
        if (!compAndStatus->_active) {
            return std::weak_ptr<T>();
        }
        std::shared_ptr<T> comp = std::dynamic_pointer_cast<T>(compAndStatus->_c);
        return comp;
    }

    void Update(float dt);

    void EditModeUpdate(float dt);

    void Destroy();
    // Used when deleting/reconnecting entities in edit mode. can deregister stuff from renderer here,
    // but maybe leave out game logic like OnDestroyEvents.
    void EditDestroy();

    // TODO: MAYBE MAKE THIS PRIVATE
    void ResetWithoutComponentDestroy();

    // NOTE: DOES NOT CALL DESTROY ON REMOVED COMPONENT.
    void RemoveComponent(int index);

    void ConnectComponentsOrDie(EntityId id, GameManager& g);

    void ConnectComponentsOrDeactivate(
        EntityId id, GameManager& g, std::vector<ComponentType>* failures = nullptr);

    std::weak_ptr<Component> TryAddComponentOfType(ComponentType c);

    template<typename T>
    std::weak_ptr<T> TryAddComponent() {
        auto iter = _componentTypeMap.try_emplace(
            std::type_index(typeid(T)));
        if (!iter.second) {
            return std::weak_ptr<T>();
        }

        auto component = std::make_shared<T>();
        {
            auto compAndStatus = std::make_unique<ComponentAndStatus>();
            compAndStatus->_c = component;
            compAndStatus->_active = true;
            _components.push_back(std::move(compAndStatus));
        }

        iter.first->second = _components.back().get();
        return component;
    }

    template<typename T>
    std::weak_ptr<T> AddComponentOrDie() {
        std::weak_ptr<T> pComp = TryAddComponent<T>();
        assert(!pComp.expired());
        return pComp;
    }

    // FOR EDITING AND CEREALIZATION ONLY
    int GetNumComponents() const { return _components.size(); }
    Component const& GetComponent(int i) const { return *_components.at(i)->_c; }
    Component& GetComponent(int i) {
        return *_components.at(i)->_c;
    }
    void OnEditPickComponents() {
        for (auto& compAndStatus : _components) {
            if (compAndStatus->_active) {
                compAndStatus->_c->OnEditPick();
            }
        }
    }

    void Save(ptree& pt) const;
    bool Save(char const* filename) const;
    void Load(ptree const& pt);
    bool Load(char const* filename);

    std::string _name;
private:
    struct ComponentAndStatus {
        std::shared_ptr<Component> _c;
        bool _active = true;
    };
    // We need this to be a vector of pointers so that _componentTypeMap can
    // have stable pointers to the vector's data even if vector gets
    // reallocated.
    std::vector<std::unique_ptr<ComponentAndStatus>> _components;
    // TODO: remove this type_index shit
    std::unordered_map<std::type_index, ComponentAndStatus*> _componentTypeMap;
};