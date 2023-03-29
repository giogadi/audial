#pragma once

#include <vector>

#include "serial.h"
#include "matrix.h"
#include "game_manager.h"
#include "new_entity.h"

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
    bool _localToEntity = false;

    void Init(GameManager& g, ne::Entity const& entity);
    // Returns true if waypoint logic updated position
    // offset gets added to waypoint positions when drawing and populating newPos.
    bool Update(GameManager& g, float dt, bool debugDraw, ne::Entity* pEntity);
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool ImGui();

    void Start(GameManager& g, ne::Entity const& e);
    void Stop();

    struct State {
        bool _followingWaypoints = false;
        int _currentWaypointIx = 0;
        Vec3 _prevWaypointPos;
        Vec3 _entityPosAtStart;
        double _thisWpStartTime = 3.0;
    };
    State _ns;
};
