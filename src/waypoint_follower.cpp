#include "waypoint_follower.h"

#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "math_util.h"

void Waypoint::Save(serial::Ptree pt) const {
    pt.PutFloat("t", _t);
    serial::SaveInNewChildOf(pt, "p", _p);
}

void Waypoint::Load(serial::Ptree pt) {
    _t = pt.GetFloat("t");
    if (!serial::LoadFromChildOf(pt, "p", _p)) {
        printf("Waypoint::Load: ERROR couldn't find child \"p\"\n");
    }
}

bool Waypoint::ImGui() {
    ImGui::InputFloat("Time", &_t);
    imgui_util::InputVec3("Pos", &_p);
    return false;
}

void WaypointFollower::Init(Vec3 const& initPos) {
    _currentWaypointTime = 0.f;
    _currentWaypointIx = 0;
    _followingWaypoints = _autoStartFollowingWaypoints;
    _prevWaypointPos = initPos;
}

bool WaypointFollower::Update(float const dt, Vec3* newPos) {
    if (!_followingWaypoints || _waypoints.empty()) {
        return false;
    }
    
    _currentWaypointTime += dt;
    assert(_currentWaypointIx < _waypoints.size());            
    if (_currentWaypointTime > _waypoints[_currentWaypointIx]._t) {  
        _prevWaypointPos = _waypoints[_currentWaypointIx]._p;
        _currentWaypointTime -= _waypoints[_currentWaypointIx]._t;
        ++_currentWaypointIx;
        if (_currentWaypointIx >= _waypoints.size()) {
            if (_loopWaypoints) {
                _currentWaypointIx = 0;
            } else {
                --_currentWaypointIx;
                _currentWaypointTime = _waypoints[_currentWaypointIx]._t;
            }
        }
    }
    float lerpFactor = _currentWaypointTime / _waypoints[_currentWaypointIx]._t;
    lerpFactor = math_util::Clamp(lerpFactor, 0.f, 1.f);
    // lerp from prevWaypointPos to pos of current waypoint
    Waypoint const& currentWp = _waypoints[_currentWaypointIx];
    *newPos = _prevWaypointPos + lerpFactor * (currentWp._p - _prevWaypointPos);
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
