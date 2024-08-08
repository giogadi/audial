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
    // pt.PutBool("local_to_entity", _localToEntity);
    pt.PutBool("wp_relative", _wpRelative);
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
    // pt.TryGetBool("local_to_entity", &_localToEntity);
    pt.TryGetBool("wp_relative", &_wpRelative);
    pt.TryGetBool("waypoint_auto_start", &_autoStartFollowingWaypoints);
    pt.TryGetBool("loop_waypoints", &_loopWaypoints);
    pt.TryGetDouble("start_time", &_initWpStartTime);
    pt.TryGetDouble("quantize", &_startQuantize);
    pt.TryGetDouble("quantize_buffer", &_quantizeBufferTime);
    serial::LoadVectorFromChildNode(pt, "waypoints", _waypoints);

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
        changed = ImGui::Checkbox("Relative", &_wpRelative) || changed;
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
        // if (p._localToEntity) {
        //     _ns._prevWaypointPos.Set(0.f, 0.f, 0.f);
        // }
        // else {
        //     _ns._prevWaypointPos = entity._transform.Pos();
        // }
        _ns._prevWaypointPos = entity._transform.Pos();
        _ns._thisWpStartTime = p._initWpStartTime;

        if (g._editMode) {
            _ns._entityPosAtStart = entity._transform.Pos();
        }
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
        _ns._thisWpStartTime = prevQuantize;
    } else {
        _ns._thisWpStartTime = nextQuantize;
    }
    _ns._entityPosAtStart = entity._transform.Pos();
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
            Vec3 offset;
            if (p._wpRelative) {
                // offset = _ns._entityPosAtStart;
                offset = _ns._prevWaypointPos;
            } else {
                offset = _ns._entityPosAtStart;
            }
            for (int ii = 0; ii < p._waypoints.size(); ++ii) {
                Waypoint const& wp = p._waypoints[ii];
                Vec3 worldPos = wp._p + offset;
                trans.SetTranslation(worldPos);
                g._scene->DrawBoundingBox(trans, wpColor);                
            }
        }
        return false;
    }
    
    if (!_ns._followingWaypoints || p._waypoints.empty()) {
        return false;
    }

    Vec3 newPos;

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    assert(_ns._currentWaypointIx >= 0 && _ns._currentWaypointIx < p._waypoints.size());
    Waypoint const& wp = p._waypoints[_ns._currentWaypointIx];
    if (beatTime >= _ns._thisWpStartTime + wp._waitTime + wp._moveTime) {
        if (p._wpRelative) {
            newPos = wp._p + _ns._prevWaypointPos;
        } else {
            newPos = wp._p + _ns._entityPosAtStart;    
        }
        // newPos = wp._p;
        // _ns._prevWaypointPos = wp._p;
        _ns._prevWaypointPos = newPos;
        ++_ns._currentWaypointIx;
        if (_ns._currentWaypointIx >= p._waypoints.size()) {
            if (p._loopWaypoints) {
                // TODO I don't think this works lol
                _ns._currentWaypointIx = 0;
            }
            else {
                _ns._currentWaypointIx = p._waypoints.size() - 1;
                _ns._followingWaypoints = false;
            }
        }
        _ns._thisWpStartTime += wp._waitTime + wp._moveTime;
    } else if (beatTime > _ns._thisWpStartTime + wp._waitTime) {
        // Moving toward the next waypoint!
        // Need to go from [0,moveTime] -> [0,1]
        double elapsedMoveTime = beatTime - (_ns._thisWpStartTime + wp._waitTime);
        float factor = (float)(elapsedMoveTime / wp._moveTime);
        factor = math_util::Clamp(factor, 0.f, 1.f);
        Vec3 wpPos;
        if (p._wpRelative) {
            wpPos = wp._p + _ns._prevWaypointPos;
        } else {
            wpPos = wp._p + _ns._entityPosAtStart;
        }
        // Vec3 wpPos = wp._p;
        newPos = _ns._prevWaypointPos + factor * (wpPos - _ns._prevWaypointPos);
    } else {
        // just wait
        return false;
    }

    // if (p._localToEntity) {
    //     newPos += _ns._entityPosAtStart;
    // }
    pEntity->_transform.SetPos(newPos);
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
