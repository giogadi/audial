#include "flow_wall.h"

#include "game_manager.h"
#include "beat_clock.h"
#include "math_util.h"

void FlowWallEntity::InitDerived(GameManager& g) {
    _wpFollower.Init(_transform.Pos());
}

void FlowWallEntity::Update(GameManager& g, float dt) {

    if (!g._editMode && !_wpFollower._waypoints.empty()) {
        // Vec3 newPos;
        // if (_wpFollower.Update(dt, &newPos)) {
        //     _transform.SetPos(newPos);
        // }

        // Assume for now that we are to hit each waypoint on a downbeat. only
        // start moving on an upbeat. We'll generalize from there.
        double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
        if (beatTime >= _wpArriveTime) {            
            _transform.SetPos(_wpFollower._waypoints[_wpFollower._currentWaypointIx]._p);
            _wpFollower._prevWaypointPos = _transform.Pos();
            ++_wpFollower._currentWaypointIx;
            if (_wpFollower._currentWaypointIx >= _wpFollower._waypoints.size()) {
                _wpFollower._currentWaypointIx = 0;
            }
            _wpArriveTime += 1.0;
        } else if (beatTime >= _wpArriveTime - 1.0) {
            double const beatFrac = g._beatClock->GetBeatFraction(beatTime);
            if (beatFrac >= 0.75) {
                // We're moving toward the next waypoint!
                // Need to go from [0.5,1] -> [0,1]
                float factor = 2.f * ((float)beatFrac - 0.75f);
                factor = math_util::Clamp(factor, 0.f, 1.f);
                Vec3 wpPos = _wpFollower._waypoints[_wpFollower._currentWaypointIx]._p;
                Vec3 newPos = _wpFollower._prevWaypointPos + factor * (wpPos - _wpFollower._prevWaypointPos);
                _transform.SetPos(newPos);
            }
        } else {
            // just wait
        }
    }

    BaseEntity::Update(g, dt);
}

void FlowWallEntity::SaveDerived(serial::Ptree pt) const {
    _wpFollower.Save(pt);
}

void FlowWallEntity::LoadDerived(serial::Ptree pt) {
    _wpFollower.Load(pt);
}

FlowWallEntity::ImGuiResult FlowWallEntity::ImGuiDerived(GameManager& g)  {
    _wpFollower.ImGui();
    return ImGuiResult::Done;
}
