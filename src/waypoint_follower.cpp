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
    _ns = State();
    
    if (_autoStartFollowingWaypoints) {
        Start(g, entity);
    }
    if (_localToEntity) {
        _ns._prevWaypointPos.Set(0.f, 0.f, 0.f);
    } else {
        _ns._prevWaypointPos = entity._transform.Pos();
    }
    _ns._thisWpStartTime = _initWpStartTime;

    if (g._editMode) {
        _ns._entityPosAtStart = entity._transform.Pos();
    }
}

void WaypointFollower::Start(GameManager& g, ne::Entity const& entity) {
    _ns._followingWaypoints = true;
    _ns._thisWpStartTime = BeatClock::GetNextBeatDenomTime(g._beatClock->GetBeatTimeFromEpoch(), 1.0);
    _ns._entityPosAtStart = entity._transform.Pos();
}

void WaypointFollower::Stop() {
    _ns._followingWaypoints = false;
}

bool WaypointFollower::Update(GameManager& g, float const dt, bool debugDraw, ne::Entity* pEntity) {
    if (g._editMode) {
        if (debugDraw) {
            Mat4 trans;
            trans.ScaleUniform(0.5f);
            trans.Scale(1.f, 10.f, 1.f);
            Vec4 wpColor(0.8f, 0.f, 0.8f, 1.f);
            Vec3 offset;
            if (_localToEntity) {
                offset = _ns._entityPosAtStart;
            }
            for (int ii = 0; ii < _waypoints.size(); ++ii) {
                Waypoint const& wp = _waypoints[ii];
                Vec3 worldPos = wp._p + offset;
                trans.SetTranslation(worldPos);
                g._scene->DrawBoundingBox(trans, wpColor);                
            }
        }
        return false;
    }
    
    if (!_ns._followingWaypoints || _waypoints.empty()) {
        return false;
    }

    Vec3 newPos;

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    assert(_ns._currentWaypointIx >= 0 && _ns._currentWaypointIx < _waypoints.size());
    Waypoint const& wp = _waypoints[_ns._currentWaypointIx];
    if (beatTime >= _ns._thisWpStartTime + wp._waitTime + wp._moveTime) {
        newPos = wp._p;
        _ns._prevWaypointPos = wp._p;
        ++_ns._currentWaypointIx;
        if (_ns._currentWaypointIx >= _waypoints.size()) {
            _ns._currentWaypointIx = 0;
        }
        _ns._thisWpStartTime += wp._waitTime + wp._moveTime;
    } else if (beatTime > _ns._thisWpStartTime + wp._waitTime) {
        // Moving toward the next waypoint!
        // Need to go from [0,moveTime] -> [0,1]
        double elapsedMoveTime = beatTime - (_ns._thisWpStartTime + wp._waitTime);
        float factor = (float)(elapsedMoveTime / wp._moveTime);
        factor = math_util::Clamp(factor, 0.f, 1.f);
        Vec3 wpPos = wp._p;
        newPos = _ns._prevWaypointPos + factor * (wpPos - _ns._prevWaypointPos);
    } else {
        // just wait
        return false;
    }

    if (_localToEntity) {
        newPos += _ns._entityPosAtStart;
    }
    pEntity->_transform.SetPos(newPos);
    return true;
}

void WaypointFollower::Save(serial::Ptree pt) const {
    pt.PutBool("local_to_entity", _localToEntity);
    pt.PutBool("waypoint_auto_start", _autoStartFollowingWaypoints);
    pt.PutBool("loop_waypoints", _loopWaypoints);
    pt.PutDouble("start_time", _initWpStartTime);
    serial::SaveVectorInChildNode(pt, "waypoints", "waypoint", _waypoints);
}

void WaypointFollower::Load(serial::Ptree pt) {
    _localToEntity = false;
    pt.TryGetBool("local_to_entity", &_localToEntity);
    _autoStartFollowingWaypoints = false;
    pt.TryGetBool("waypoint_auto_start", &_autoStartFollowingWaypoints);
    _loopWaypoints = false;
    pt.TryGetBool("loop_waypoints", &_loopWaypoints);
    if (!pt.TryGetDouble("start_time", &_initWpStartTime)) {
        _initWpStartTime = 3.0; // previous default, before we serialized it
    }
    serial::LoadVectorFromChildNode(pt, "waypoints", _waypoints);
}

bool WaypointFollower::ImGui() {
    ImGui::Checkbox("Local to Entity", &_localToEntity);
    ImGui::Checkbox("Waypoint Auto Start", &_autoStartFollowingWaypoints);
    if (_autoStartFollowingWaypoints) {
        ImGui::InputDouble("Start time", &_initWpStartTime);
    }
    ImGui::Checkbox("Loop Waypoints", &_loopWaypoints);
    bool changed = imgui_util::InputVector(_waypoints);
    return changed;
}
