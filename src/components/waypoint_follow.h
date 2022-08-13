#pragma once

#include <vector>

#include "component.h"

class TransformComponent;
class BoundMeshPNU;
namespace renderer {
    class ColorModelInstance;
};

class WaypointFollowComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::WaypointFollow; }
    virtual void Update(float dt) override;
    virtual void EditModeUpdate(float dt) override;
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    virtual bool DrawImGui() override;
    virtual void Save(serial::Ptree pt) const override;
    virtual void Load(serial::Ptree pt) override;


    // Serialized
    std::vector<std::string> _waypointNames;
    bool _loop = false;
    float _speed = 1.f;
    bool _autoStart = true;

    // Not serialized
    std::weak_ptr<TransformComponent> _t;
    std::vector<EntityId> _waypointIds;
    int _currentWaypointIx = 0;
    GameManager* _g;
    BoundMeshPNU const* _debugMesh = nullptr;
    bool _running = false;

private:
    void DrawLinesThroughWaypoints();
};