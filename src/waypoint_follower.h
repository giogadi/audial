#pragma once

#include <vector>

#include "new_entity_id.h"
#include "editor_id.h"
#include "serial.h"
#include "matrix.h"

namespace ne {
    struct BaseEntity;
}
struct GameManager;

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
    struct Props {
        enum class Mode { Waypoint, FollowEntity };
        Mode _mode = Mode::Waypoint;
        // Waypoint mode
        std::vector<Waypoint> _waypoints;
        bool _autoStartFollowingWaypoints = false;
        bool _loopWaypoints = false;
        double _initWpStartTime = 3.0;
        double _startQuantize = 1.0;
        double _quantizeBufferTime = 0.0;
        // FollowEntity mode
        EditorId _followEditorId;
        std::string _followEntityName;
        void Save(serial::Ptree pt) const;
        void Load(serial::Ptree pt);
        bool ImGui();
    };

    void Init(GameManager& g, ne::BaseEntity const& entity, Props const& p);
    // Returns true if waypoint logic updated position
    // offset gets added to waypoint positions when drawing and populating newPos.
    bool Update(GameManager& g, float dt, ne::BaseEntity* pEntity, Props const& p);
    bool UpdateWaypoint(GameManager& g, float dt, ne::BaseEntity* pEntity, Props const& p);
    bool UpdateFollowEntity(GameManager& g, float const dt, ne::BaseEntity* pEntity, Props const& p);

    void Start(GameManager& g, ne::BaseEntity const& e);
    void Stop();

    struct State {
        bool _followingWaypoints = false;
        int _currentWaypointIx = -1;
        double _nextWpStartTime = 3.0;
        double _thisWpStartTime = -1.0;
        Vec3 _currentMotion;
        float _lastMotionFactor = 0.f;

        ne::EntityId _followEntityId;
        Vec3 _offsetFromFollowEntity;
    };
    State _ns;
};
