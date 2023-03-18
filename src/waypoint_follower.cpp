#include "waypoint_follower.h"

#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "math_util.h"
#include "beat_clock.h"

void Waypoint::Save(serial::Ptree pt) const {
    pt.PutDouble("wait_time", _waitTime);
    pt.PutDouble("move_time", _moveTime);    
    serial::SaveInNewChildOf(pt, "p", _p);
}

void Waypoint::Load(serial::Ptree pt) {
    pt.TryGetDouble("wait_time", &_waitTime);
    pt.TryGetDouble("move_time", &_moveTime);
    if (!serial::LoadFromChildOf(pt, "p", _p)) {
        printf("Waypoint::Load: ERROR couldn't find child \"p\"\n");
    }
}

bool Waypoint::ImGui() {
    ImGui::InputDouble("Wait time", &_waitTime);
    ImGui::InputDouble("Move time", &_moveTime);
    imgui_util::InputVec3("Pos", &_p);
    return false;
}

void WaypointFollower::Init(Vec3 const& initPos) {
    _currentWaypointIx = 0;
    _followingWaypoints = _autoStartFollowingWaypoints;
    _prevWaypointPos = initPos;
    _thisWpStartTime = 3.0;
}

bool WaypointFollower::Update(GameManager& g, float const dt, Vec3* newPos) {
    if (/*!_followingWaypoints || */_waypoints.empty()) {
        return false;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    assert(_currentWaypointIx >= 0 && _currentWaypointIx < _waypoints.size());
    Waypoint const& wp = _waypoints[_currentWaypointIx];
    if (beatTime >= _thisWpStartTime + wp._waitTime + wp._moveTime) {
        *newPos = wp._p;
        _prevWaypointPos = wp._p;
        ++_currentWaypointIx;
        if (_currentWaypointIx >= _waypoints.size()) {
            _currentWaypointIx = 0;
        }
        _thisWpStartTime += wp._waitTime + wp._moveTime;
    } else if (beatTime > _thisWpStartTime + wp._waitTime) {
        // Moving toward the next waypoint!
        // Need to go from [0,moveTime] -> [0,1]
        double elapsedMoveTime = beatTime - (_thisWpStartTime + wp._waitTime);
        float factor = (float)(elapsedMoveTime / wp._moveTime);
        factor = math_util::Clamp(factor, 0.f, 1.f);
        Vec3 wpPos = wp._p;
        *newPos = _prevWaypointPos + factor * (wpPos - _prevWaypointPos);
    } else {
        // just wait
        return false;
    }
    return true;
}

void WaypointFollower::Save(serial::Ptree pt) const {
    pt.PutBool("waypoint_auto_start", _autoStartFollowingWaypoints);
    pt.PutBool("loop_waypoints", _loopWaypoints);
    serial::SaveVectorInChildNode(pt, "waypoints", "waypoint", _waypoints);
}

void WaypointFollower::Load(serial::Ptree pt) {
    _autoStartFollowingWaypoints = false;
    pt.TryGetBool("waypoint_auto_start", &_autoStartFollowingWaypoints);
    _loopWaypoints = false;
    pt.TryGetBool("loop_waypoints", &_loopWaypoints);
    serial::LoadVectorFromChildNode(pt, "waypoints", _waypoints);
}

bool WaypointFollower::ImGui() {
    ImGui::Checkbox("Waypoint Auto Start", &_autoStartFollowingWaypoints);
    ImGui::Checkbox("Loop Waypoints", &_loopWaypoints);
    bool changed = imgui_util::InputVector(_waypoints);
    return changed;
}
