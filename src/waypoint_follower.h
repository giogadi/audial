#pragma once

#include <vector>

#include "serial.h"
#include "matrix.h"

struct Waypoint {
    float _t;  // time in seconds to get to this point from the previous one.
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
    bool Update(float dt, Vec3* newPos);
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool ImGui();

    // non-serialized
    float _currentWaypointTime = 0.f;
    bool _followingWaypoints = false;
    int _currentWaypointIx = 0;
    Vec3 _prevWaypointPos;
};
