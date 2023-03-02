#pragma once

#include "new_entity.h"
#include "waypoint_follower.h"
#include "seq_action.h"

struct FlowWallEntity : public ne::Entity {
    // serialized
    WaypointFollower _wpFollower;
    double _wpArriveTime = 4.0;
    std::vector<std::string> _hitActionStrings;

    // non-serialized
    std::vector<std::unique_ptr<SeqAction>> _hitActions;

    void OnHit(GameManager& g);
       
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    FlowWallEntity() = default;
    FlowWallEntity(FlowWallEntity const&) = delete;
    FlowWallEntity(FlowWallEntity&&) = default;
    FlowWallEntity& operator=(FlowWallEntity&&) = default;
};
