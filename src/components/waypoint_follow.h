#pragma once

#include "component.h"

class TransformComponent;
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
    virtual void Save(boost::property_tree::ptree& pt) const override;
    virtual void Load(boost::property_tree::ptree const& pt) override;
    virtual void EditDestroy() override;

    // Serialized
    std::vector<std::string> _waypointNames;

    // Not serialized
    std::weak_ptr<TransformComponent> _t;
    std::vector<EntityId> _waypointIds;
    std::vector<VersionId> _wpModelIds;
    int _currentWaypointIx = 0;
    GameManager* _g;
};