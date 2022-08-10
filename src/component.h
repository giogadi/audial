#pragma once

// TODO: try to remove this include
#include "boost/property_tree/ptree.hpp"

#include "entity_id.h"

using boost::property_tree::ptree;

class Entity;
class EntityManager;
class GameManager;

#define M_COMPONENT_TYPES \
    X(Transform) \
    X(Velocity) \
    X(RigidBody) \
    X(Light) \
    X(Camera) \
    X(PlayerController) \
    X(BeepOnHit) \
    X(Sequencer) \
    X(PlayerOrbitController) \
    X(CameraController) \
    X(HitCounter) \
    X(Orbitable) \
    X(EventsOnHit) \
    X(Activator) \
    X(Damage) \
    X(OnDestroyEvent) \
    X(WaypointFollow) \
    X(RecorderBalloon) \
    X(Model)

enum class ComponentType: int {
#   define X(a) a,
    M_COMPONENT_TYPES
#   undef X
    NumTypes
};

extern char const* gComponentTypeStrings[];

// TODO: can make this constexpr?
char const* ComponentTypeToString(ComponentType c);

ComponentType StringToComponentType(char const* s);

class Component {
public:
    virtual ~Component() {}
    virtual void Update(float dt) {}
    virtual void Destroy() {};
    virtual void EditDestroy() { Destroy(); };
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) { return true; }
    virtual ComponentType Type() const = 0;
    // If true, request that we try reconnecting the entity's components.
    virtual bool DrawImGui();
    virtual void OnEditPick() {}
    virtual void EditModeUpdate(float dt) {}
    virtual void Save(boost::property_tree::ptree& pt) const {}
    virtual void Load(boost::property_tree::ptree const& pt) {}
};