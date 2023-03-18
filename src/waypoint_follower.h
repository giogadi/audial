#pragma once

#include <vector>

#include "serial.h"
#include "matrix.h"
#include "game_manager.h"

struct Waypoint {
    double _waitTime = 0.75f;
    double _moveTime = 0.25f;
    Vec3 _p;
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool ImGui();
};

struct WaypointFollower {
    // serialized   
    std::vector<Waypoint> _waypoints;
    bool _autoStartFollowingWaypoints = false;
    bool _loopWaypoints = false;

    void Init(Vec3 const& initPos);
    // Returns true if waypoint logic updated position
    bool Update(GameManager& g, float dt, Vec3* newPos);
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool ImGui();

    // non-serialized
    bool _followingWaypoints = false;
    int _currentWaypointIx = 0;
    Vec3 _prevWaypointPos;
    double _thisWpStartTime = 3.0;
};
