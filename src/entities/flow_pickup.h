#pragma once

#include "new_entity.h"
#include "waypoint_follower.h"

struct FlowPickupEntity : public ne::Entity {
    // serialized
    // WaypointFollower _wpFollower;
    // double _wpArriveTime = 4.0;
       
    // virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    // virtual void InitDerived(GameManager& g) override;
    // virtual void SaveDerived(serial::Ptree pt) const override;
    // virtual void LoadDerived(serial::Ptree pt) override;
    // virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};