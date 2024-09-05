#include "waypoint_follower.h"

#include "util.h"
#include "game_manager.h"
#include "new_entity.h"
#include "editor.h"
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
    *this = Waypoint();
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

void WaypointFollower::Props::Save(serial::Ptree pt) const {
    switch (_mode) {
    case Props::Mode::Waypoint: {
        pt.PutString("mode", "Waypoint");
        break;
    }
    case Props::Mode::FollowEntity: {
        pt.PutString("mode", "FollowEntity");
        break;
    }
    }
    pt.PutBool("waypoint_auto_start", _autoStartFollowingWaypoints);
    pt.PutBool("loop_waypoints", _loopWaypoints);
    pt.PutDouble("start_time", _initWpStartTime);
    pt.PutDouble("quantize", _startQuantize);
    pt.PutDouble("quantize_buffer", _quantizeBufferTime);
    serial::SaveVectorInChildNode(pt, "waypoints", "waypoint", _waypoints);

    pt.PutString("follow_entity_name", _followEntityName.c_str());
    serial::SaveInNewChildOf(pt, "follow_entity_editor_id", _followEditorId);    
}

void WaypointFollower::Props::Load(serial::Ptree pt) {
    *this = WaypointFollower::Props();
    std::string modeStr;
    if (pt.TryGetString("mode", &modeStr)) {
        if (modeStr == "Waypoint") {
            _mode = Props::Mode::Waypoint;
        }
        else if (modeStr == "FollowEntity") {
            _mode = Props::Mode::FollowEntity;
        }
        else {
            printf("WaypointFollower::Props::Load: unrecognized enum \"%s\"\n", modeStr.c_str());
        }
    }
    int const kRelativeDefaultVersion = 8;
    bool relative = true;
    if (pt.GetVersion() < kRelativeDefaultVersion) {
        // Relative used to not be the default (was kinda absolute before sorta)
        relative = false;
        pt.TryGetBool("wp_relative", &relative);
    }
    pt.TryGetBool("waypoint_auto_start", &_autoStartFollowingWaypoints);
    pt.TryGetBool("loop_waypoints", &_loopWaypoints);
    pt.TryGetDouble("start_time", &_initWpStartTime);
    pt.TryGetDouble("quantize", &_startQuantize);
    pt.TryGetDouble("quantize_buffer", &_quantizeBufferTime);
    serial::LoadVectorFromChildNode(pt, "waypoints", _waypoints);

    if (!relative && !_waypoints.empty()) {
        // If loading an old script that had "absolute" wp positions, change the positions so that they are relative.
        std::vector<Vec3> relativePos(_waypoints.size());
        relativePos[0] = _waypoints[0]._p;
        for (int ii = 0; ii < _waypoints.size() - 1; ++ii) {
            relativePos[ii + 1] = _waypoints[ii + 1]._p - _waypoints[ii]._p;
        }
        for (int ii = 0; ii < _waypoints.size(); ++ii) {
            _waypoints[ii]._p = relativePos[ii];
        }
    }

    pt.TryGetString("follow_entity_name", &_followEntityName);
    serial::LoadFromChildOf(pt, "follow_entity_editor_id", _followEditorId);
}

namespace {
char const* kModeStrings[] = {
    "Waypoint",
    "FollowEntity"
};

std::vector<Waypoint> sClipboardWaypoints;
}

bool WaypointFollower::Props::ImGui() {
    bool changed = false;
    {
        int selectedModeIx = static_cast<int>(_mode);
        bool modeChanged = ImGui::Combo("Mode", &selectedModeIx, kModeStrings, static_cast<int>(M_ARRAY_LEN(kModeStrings)));
        if (modeChanged) {
            _mode = static_cast<Props::Mode>(selectedModeIx);
        }
        changed = changed || modeChanged;
    }
    switch (_mode) {
    case Props::Mode::Waypoint: {
        changed = ImGui::Checkbox("Waypoint Auto Start", &_autoStartFollowingWaypoints) || changed;
        if (_autoStartFollowingWaypoints) {
            changed = ImGui::InputDouble("Start time", &_initWpStartTime) || changed;
        }
        changed = ImGui::InputDouble("Quantize", &_startQuantize) || changed;
        changed = ImGui::InputDouble("Quantize buffer", &_quantizeBufferTime) || changed;
        changed = ImGui::Checkbox("Loop Waypoints", &_loopWaypoints) || changed;
        if (ImGui::Button("Clear Waypoints")) {
            _waypoints.clear();
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy Waypoints")) {
            sClipboardWaypoints = _waypoints;
        }
        ImGui::SameLine();
        if (ImGui::Button("Paste Waypoints")) {
            _waypoints = sClipboardWaypoints;
            changed = true;
        }
        changed = imgui_util::InputVector(_waypoints, imgui_util::InputVectorOptions()) || changed;
        break;
    }
    case Props::Mode::FollowEntity: {
        changed = imgui_util::InputEditorId("Follow entity ID", &_followEditorId) || changed;
        break;
    }
    }
    
    return changed;
}

void WaypointFollower::Init(GameManager& g, ne::BaseEntity const& entity, Props const& p) {
    _ns = State();

    switch (p._mode) {
    case Props::Mode::Waypoint: {
        if (p._autoStartFollowingWaypoints) {
            Start(g, entity);
        }
        _ns._nextWpStartTime = p._initWpStartTime;

        break;
    }
    case Props::Mode::FollowEntity: {
        ne::BaseEntity* e = g._neEntityManager->FindEntityByEditorId(p._followEditorId);
        if (e) {
            _ns._followEntityId = e->_id;
            _ns._offsetFromFollowEntity = entity._initTransform.GetPos() - e->_initTransform.GetPos();
        }
        break;
    }
    }    
}

void WaypointFollower::Start(GameManager& g, ne::BaseEntity const& entity) {
    _ns._followingWaypoints = true;
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double denom = entity._wpProps._startQuantize;
    double nextQuantize = BeatClock::GetNextBeatDenomTime(beatTime, denom);
    double prevQuantize = nextQuantize - denom;
    if (beatTime - prevQuantize <= entity._wpProps._quantizeBufferTime) {
        _ns._nextWpStartTime = prevQuantize;
    } else {
        _ns._nextWpStartTime = nextQuantize;
    }
}

void WaypointFollower::Stop() {
    _ns._followingWaypoints = false;
}

bool WaypointFollower::Update(GameManager& g, float const dt, ne::BaseEntity* pEntity, Props const& p) {
    switch (p._mode) {
    case Props::Mode::Waypoint: {
        return UpdateWaypoint(g, dt, pEntity, p);
        break;
    }
    case Props::Mode::FollowEntity: {
        return UpdateFollowEntity(g, dt, pEntity, p);
        break;
    }
    }
    assert(false);
    return false;
}

bool WaypointFollower::UpdateWaypoint(GameManager& g, float const dt, ne::BaseEntity* pEntity, Props const& p) {
    if (g._editMode) {
        bool const editModeSelected = pEntity && g._editor->IsEntitySelected(pEntity->_id);
        if (editModeSelected) {
            Mat4 trans;
            trans.ScaleUniform(0.5f);
            trans.Scale(1.f, 10.f, 1.f);
            Vec4 wpColor(0.8f, 0.f, 0.8f, 1.f);
            Vec3 wpPos = pEntity->_initTransform.Pos();
            for (int ii = 0; ii < p._waypoints.size(); ++ii) {
                Waypoint const& wp = p._waypoints[ii];
                wpPos += wp._p;
                trans.SetTranslation(wpPos);
                g._scene->DrawBoundingBox(trans, wpColor);                
            }
        }
        return false;
    }
    
    if (!_ns._followingWaypoints || p._waypoints.empty()) {
        return false;
    }    

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    Vec3 dp;

    if (beatTime >= _ns._nextWpStartTime) {
        if (_ns._currentWaypointIx < 0) {
            _ns._currentWaypointIx = 0;
        } else {
            // Finish the motion.
            Vec3 prevLocalPos = _ns._lastMotionFactor * _ns._currentMotion;
            Vec3 localPos = _ns._currentMotion;
            dp = localPos - prevLocalPos;

            ++_ns._currentWaypointIx;
            if (_ns._currentWaypointIx >= p._waypoints.size()) {
                if (p._loopWaypoints) {
                    _ns._currentWaypointIx = 0;
                } else {
                    _ns._currentWaypointIx = p._waypoints.size() - 1;
                    _ns._followingWaypoints = false;
                }
            }            
        }

        if (!_ns._followingWaypoints) {
            return false;
        }

        Waypoint const& wp = p._waypoints[_ns._currentWaypointIx];
        _ns._thisWpStartTime = _ns._nextWpStartTime;
        _ns._nextWpStartTime = _ns._thisWpStartTime + wp._waitTime + wp._moveTime;
        if (beatTime >= _ns._nextWpStartTime) {
            printf("WaypointFollower: next waypoint was too short!!!\n");
        }
        _ns._lastMotionFactor = 0.0;
        _ns._currentMotion = wp._p;
    }

    if (_ns._currentWaypointIx < 0) {
        return false;
    }

    Waypoint const& wp = p._waypoints[_ns._currentWaypointIx];
    if (beatTime >= _ns._thisWpStartTime + wp._waitTime) {
        // Moving toward the next waypoint!
        // Need to go from [0,moveTime] -> [0,1]
        double elapsedMoveTime = beatTime - (_ns._thisWpStartTime + wp._waitTime);
        float factor = (float)(elapsedMoveTime / wp._moveTime);
        factor = math_util::Clamp(factor, 0.f, 1.f);

        Vec3 prevLocalPos = _ns._lastMotionFactor * _ns._currentMotion;
        Vec3 localPos = factor * _ns._currentMotion;
        dp += localPos - prevLocalPos;

        _ns._lastMotionFactor = factor;
    } else {
        // Just wait
    }
    
    pEntity->_transform.Translate(dp);
    
    return true;
}

bool WaypointFollower::UpdateFollowEntity(GameManager& g, float const dt, ne::BaseEntity* pEntity, Props const& p) {
    if (g._editMode) {
        return false;
    }
    ne::BaseEntity* followEntity = g._neEntityManager->GetEntity(_ns._followEntityId);
    if (followEntity == nullptr) {
        return false;
    }

    Vec3 newPos = followEntity->_transform.GetPos() + _ns._offsetFromFollowEntity;
    if (g._editMode && pEntity && g._editor->IsEntitySelected(pEntity->_id)) {
        Mat4 trans;
        trans.ScaleUniform(0.5f);
        trans.Scale(1.f, 10.f, 1.f);
        trans.SetTranslation(newPos);
        Vec4 wpColor(0.8f, 0.f, 0.8f, 1.f);
        g._scene->DrawBoundingBox(trans, wpColor);
        return false;
    }

    if (pEntity) {
        pEntity->_transform.SetPos(newPos);
    }    
    return true;
}
