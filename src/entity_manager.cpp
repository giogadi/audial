#include "entity_manager.h"

#include <iostream>

#include "boost/property_tree/xml_parser.hpp"

EntityId EntityManager::AddEntity(bool active) {
    EntityAndStatus* e_s;
    if (_freeList.empty()) {
        _entities.emplace_back();
        e_s = &_entities.back();
        e_s->_id = EntityId(_entities.size() - 1, 0);
    } else {
        EntityIndex freeIndex = _freeList.back();
        _freeList.pop_back();
        e_s = &_entities[freeIndex];
        EntityId newId(freeIndex, e_s->_id.GetVersion());
        e_s->_id = newId;
    }
    e_s->_e = std::make_shared<Entity>();
    e_s->_active = active;
    e_s->_destroy = EntityDestroyType::None;
    return e_s->_id;
}

void EntityManager::TagEntityForDestroy(
    EntityId idToDestroy, EntityDestroyType destroyType) {
    if (!idToDestroy.IsValid()) {
        return;
    }
    EntityIndex indexToDestroy = idToDestroy.GetIndex();
    if (indexToDestroy >= _entities.size()) {
        return;
    }
    EntityAndStatus& e_s = _entities[indexToDestroy];
    if (e_s._id != idToDestroy) {
        std::cout << "ALREADY DESTROYED THIS DUDE" << std::endl;
        return;
    }
    e_s._destroy = destroyType;
}

void EntityManager::Update(float dt) {
    ForEveryActiveEntity([this, dt](EntityId id) {
        this->GetEntity(id)->Update(dt);
    });
}

void EntityManager::EditModeUpdate(float dt) {
    ForEveryActiveEntity([this, dt](EntityId id) {
        this->GetEntity(id)->EditModeUpdate(dt);
    });
}

EntityId EntityManager::FindActiveEntityByName(char const* name) {
    for (auto const& entity : _entities) {
        if (entity._id.IsValid() && entity._active && entity._e->_name == name) {
            return entity._id;
        }
    }
    return EntityId::InvalidId();
}

EntityId EntityManager::FindInactiveEntityByName(char const* name) {
    for (auto const& entity : _entities) {
        if (entity._id.IsValid() && !entity._active && entity._e->_name == name) {
            return entity._id;
        }
    }
    return EntityId::InvalidId();
}

void EntityManager::DeactivateEntity(EntityId id) {
    EntityAndStatus* e_s = GetEntityAndStatus(id);
    if (e_s == nullptr) {
        return;
    }
    if (!e_s->_active) {
        return;
    }
    ptree entityData;
    e_s->_e->Save(entityData);
    e_s->_e->EditDestroy();
    e_s->_e->Load(entityData);
    e_s->_active = false;
}

void EntityManager::ActivateEntity(EntityId id, GameManager& g) {
    EntityAndStatus* e_s = GetEntityAndStatus(id);
    if (e_s == nullptr) {
        return;
    }
    if (e_s->_active) {
        return;
    }
    e_s->_e->ConnectComponentsOrDeactivate(id, g, /*failures=*/nullptr);
    e_s->_active = true;
}

void EntityManager::DestroyTaggedEntities() {
    ForEveryActiveAndInactiveEntity([this](EntityId id) {
        EntityAndStatus& e_s = *this->GetEntityAndStatus(id);
        switch (e_s._destroy) {
            case EntityDestroyType::None:
                return;
            case EntityDestroyType::DestroyComponents:
                e_s._e->Destroy();
                break;
            case EntityDestroyType::EditDestroyComponents:
                e_s._e->EditDestroy();
                break;
        }
        // TODO: We don't need to delete the entity yet, right? It should
        // get automatically deleted when we do AddEntity().
        e_s._id = EntityId(EntityIndex(-1), id.GetVersion() + 1);
        e_s._destroy = EntityDestroyType::None;
        this->_freeList.push_back(id.GetIndex());
    });
}

void EntityManager::ForEveryActiveEntity(std::function<void(EntityId)> f) const {
    for (auto const& e_s : _entities) {
        if (e_s._id.IsValid() && e_s._active) {
            f(e_s._id);
        }
    }
}

void EntityManager::ForEveryActiveAndInactiveEntity(std::function<void(EntityId)> f) const {
    for (auto const& e_s : _entities) {
        if (e_s._id.IsValid()) {
            f(e_s._id);
        }
    }
}

Entity* EntityManager::GetEntity(EntityId id) {
    EntityAndStatus* e_s = GetEntityAndStatus(id);
    if (e_s == nullptr) {
        return nullptr;
    }
    return e_s->_e.get();
}

Entity const* EntityManager::GetEntity(EntityId id) const {
    EntityAndStatus const* e_s = GetEntityAndStatus(id);
    if (e_s == nullptr) {
        return nullptr;
    }
    return e_s->_e.get();
}

bool EntityManager::IsActive(EntityId id) const {
    EntityAndStatus const* e_s = GetEntityAndStatus(id);
    if (e_s == nullptr) {
        return false;
    }
    return e_s->_active;
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

void EntityManager::Save(serial::Ptree pt) const {
    this->ForEveryActiveAndInactiveEntity([&](EntityId id) {
        Entity const* e = this->GetEntity(id);        
        serial::Ptree ePt = pt.AddChild("entity");
        ePt.PutBool("entity_active", this->IsActive(id));
        e->Save(ePt);
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

void EntityManager::Load(serial::Ptree pt) {
    serial::Ptree entitiesPt = pt.GetChild("entities");
    int numChildren = 0;
    serial::NameTreePair* children = entitiesPt.GetChildren(&numChildren);
    for (int i = 0; i < numChildren; ++i) {
        serial::Ptree& entityPt = children[i]._pt;
        bool active = entityPt.GetBool("entity_active");
        EntityId id = this->AddEntity(active);
        Entity* entity = this->GetEntity(id);
        entity->Load(entityPt);
    }
    free(children);
}

bool EntityManager::LoadAndConnect(char const* filename, bool dieOnConnectFailure, GameManager& g) {
    // std::ifstream inFile(filename);
    // if (!inFile.is_open()) {
    //     std::cout << "Couldn't open file " << filename << " for loading." << std::endl;
    //     return false;
    // }
    // ptree pt;
    // boost::property_tree::read_xml(inFile, pt);
    // this->Load(pt);
    // this->ConnectComponents(g, dieOnConnectFailure);
    // return true;

    serial::Ptree pt = serial::Ptree::MakeNew();
    if (pt.LoadFromFile(filename)) {
        this->Load(pt);
        this->ConnectComponents(g, dieOnConnectFailure);
        return true;
    }
    return false;
}

bool EntityManager::Save(char const* filename) const {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "Couldn't open file " << filename << " for saving. Not saving." << std::endl;
        return false;
    }
    ptree pt;
    {
        ptree& entitiesPt = pt.add_child("entities", ptree());
        this->Save(entitiesPt);
    }
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(outFile, pt, settings);

    {
        serial::Ptree pt = serial::Ptree::MakeNew();
        serial::Ptree entitiesPt = pt.AddChild("entities");;
        this->Save(entitiesPt);

        return pt.WriteToFile("data/pimpl_ptree_test.xml");
    }
    
    return true;
}

EntityAndStatus* EntityManager::GetEntityAndStatus(EntityId id) {
    if (id.IsValid()) {
        EntityIndex index = id.GetIndex();
        if (index < _entities.size()) {
            EntityAndStatus& e_s = _entities[index];
            if (e_s._id == id) {
                return &e_s;
            }
        }
    }
    return nullptr;
}

EntityAndStatus const* EntityManager::GetEntityAndStatus(EntityId id) const {
    if (id.IsValid()) {
        EntityIndex index = id.GetIndex();
        if (index < _entities.size()) {
            EntityAndStatus const& e_s = _entities[index];
            if (e_s._id == id) {
                return &e_s;
            }
        }
    }
    return nullptr;
}