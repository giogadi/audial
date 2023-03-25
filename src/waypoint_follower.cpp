#include "waypoint_follower.h"

#include "imgui_vector_util.h"
#include "serial_vector_util.h"
#include "math_util.h"
#include "beat_clock.h"
#include "renderer.h"

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

void WaypointFollower::Init(GameManager& g, ne::Entity const& entity) {
    _currentWaypointIx = 0;
    if (_autoStartFollowingWaypoints) {
        Start(g, entity);
    }
    if (_localToEntity) {
        _prevWaypointPos.Set(0.f, 0.f, 0.f);
    } else {
        _prevWaypointPos = entity._transform.Pos();
    }
    _thisWpStartTime = 3.0;

    if (g._editMode) {
        _entityPosAtStart = entity._transform.Pos();
    }
}

void WaypointFollower::Start(GameManager& g, ne::Entity const& entity) {
    _followingWaypoints = true;
    _thisWpStartTime = BeatClock::GetNextBeatDenomTime(g._beatClock->GetBeatTimeFromEpoch(), 1.0);
    _entityPosAtStart = entity._transform.Pos();
}

void WaypointFollower::Stop() {
    _followingWaypoints = false;
}

bool WaypointFollower::Update(GameManager& g, float const dt, bool debugDraw, ne::Entity* pEntity) {
    if (g._editMode && debugDraw) {
        Mat4 trans;
        trans.ScaleUniform(0.5f);
        Vec4 wpColor(0.8f, 0.f, 0.8f, 1.f);
        Vec3 offset;
        if (_localToEntity) {
            offset = _entityPosAtStart;
        }
        for (Waypoint const& wp : _waypoints) {
            Vec3 worldPos = wp._p + offset;
            trans.SetTranslation(worldPos);
            g._scene->DrawBoundingBox(trans, wpColor);
        }
        return false;
    }
    
    if (!_followingWaypoints || _waypoints.empty()) {
        return false;
    }

    Vec3 newPos;

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    assert(_currentWaypointIx >= 0 && _currentWaypointIx < _waypoints.size());
    Waypoint const& wp = _waypoints[_currentWaypointIx];
    if (beatTime >= _thisWpStartTime + wp._waitTime + wp._moveTime) {
        newPos = wp._p;
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
        newPos = _prevWaypointPos + factor * (wpPos - _prevWaypointPos);
    } else {
        // just wait
        return false;
    }

    if (_localToEntity) {
        newPos += _entityPosAtStart;
    }
    pEntity->_transform.SetPos(newPos);
    return true;
}

void WaypointFollower::Save(serial::Ptree pt) const {
    pt.PutBool("local_to_entity", _localToEntity);
    pt.PutBool("waypoint_auto_start", _autoStartFollowingWaypoints);
    pt.PutBool("loop_waypoints", _loopWaypoints);
    serial::SaveVectorInChildNode(pt, "waypoints", "waypoint", _waypoints);
}

void WaypointFollower::Load(serial::Ptree pt) {
    _localToEntity = false;
    pt.TryGetBool("local_to_entity", &_localToEntity);
    _autoStartFollowingWaypoints = false;
    pt.TryGetBool("waypoint_auto_start", &_autoStartFollowingWaypoints);
    _loopWaypoints = false;
    pt.TryGetBool("loop_waypoints", &_loopWaypoints);
    serial::LoadVectorFromChildNode(pt, "waypoints", _waypoints);
}

bool WaypointFollower::ImGui() {
    ImGui::Checkbox("Local to Entity", &_localToEntity);
    ImGui::Checkbox("Waypoint Auto Start", &_autoStartFollowingWaypoints);
    ImGui::Checkbox("Loop Waypoints", &_loopWaypoints);
    bool changed = imgui_util::InputVector(_waypoints);
    return changed;
}
