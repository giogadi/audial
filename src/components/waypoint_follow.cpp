#include "waypoint_follow.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "entity_manager.h"
#include "components/transform.h"
#include "renderer.h"

void WaypointFollowComponent::DrawLinesThroughWaypoints() {
    for (int i = 0, n = _waypointIds.size(); i < n - 1; ++i) {
        Entity* e = _g->_entityManager->GetEntity(_waypointIds[i]);
        if (e == nullptr) {
            continue;
        }
        Vec3 p0 = e->FindComponentOfType<TransformComponent>().lock()->GetWorldPos();

        e = _g->_entityManager->GetEntity(_waypointIds[i+1]);
        if (e == nullptr) {
            continue;
        }
        Vec3 p1 = e->FindComponentOfType<TransformComponent>().lock()->GetWorldPos();

        // Cube mesh is [-0.5,0.5]
        // We arbitrarily choose to orient the local-x of the cube to point from p0 to p1.
        Vec3 xAxis = (p1 - p0);
        float segmentLength = xAxis.Normalize();
        if (segmentLength < 0.0001f) {
            continue;
        }        

        // Find arbitrary vector orthogonal to xAxis and call it Y.
        Vec3 xOrtho = Vec3::Cross(xAxis, Vec3(0.f, 1.f, 0.f));
        if (xOrtho.Length2() < 0.0001f * 0.0001f) {
            // xAxis was super close to World Y Axis. Ok, let's try World X instead.
            xOrtho = Vec3::Cross(xAxis, Vec3(1.f, 0.f, 0.f));
        }
        Vec3 yAxis = xOrtho.GetNormalized();
        Vec3 zAxis = Vec3::Cross(xAxis, yAxis);

        Mat4 trans;
        trans.Scale(segmentLength, 0.1f, 0.1f);

        Mat4 rot;
        rot.SetTopLeftMat3(Mat3(xAxis, yAxis, zAxis));

        trans = rot * trans;

        Vec3 pos = (p0 + p1) * 0.5f;
        trans.SetTranslation(pos);

        _g->_scene->DrawMesh(_debugMesh, trans, Vec4(0.f, 0.f, 0.f, 1.f));
    }
}

void WaypointFollowComponent::Update(float dt) {
    DrawLinesThroughWaypoints();

    if (!_running) {
        return;
    }

    if (_currentWaypointIx >= _waypointIds.size()) {
        if (_loop) {
            _currentWaypointIx = 0;
        } 
        if (_currentWaypointIx >= _waypointIds.size()) {
            return;
        }
    }
    EntityId wpId = _waypointIds[_currentWaypointIx];
    Entity* wp = _g->_entityManager->GetEntity(wpId);
    assert(wp != nullptr);
    std::shared_ptr<TransformComponent> wpTrans = wp->FindComponentOfType<TransformComponent>().lock();
    Vec3 wpPos = wpTrans->GetWorldPos();
    
    std::shared_ptr<TransformComponent> myTrans = _t.lock();
    assert(myTrans != nullptr);
    Vec3 myPos = myTrans->GetWorldPos();
    Vec3 fromMeToWaypoint = wpPos - myPos;
    

    float distToWp = fromMeToWaypoint.Length();
    float stepSize = _speed * dt;
    if (stepSize > distToWp) {
        // If we're gonna step past dest wp
        if (_currentWaypointIx + 1 == _waypointIds.size()) {
            stepSize = distToWp;
        }
        ++_currentWaypointIx;        
    }
    
    if (distToWp > 0.001f) {
        Vec3 dirToWp = fromMeToWaypoint / distToWp;
        Vec3 newPos = myPos + stepSize * dirToWp;
        myTrans->SetWorldPos(newPos);
    }
}

void WaypointFollowComponent::EditModeUpdate(float dt) {
    DrawLinesThroughWaypoints();

    // Update debug models
    for (int i = 0; i < _waypointIds.size(); ++i) {
        Entity* e = _g->_entityManager->GetEntity(_waypointIds[i]);
        if (e == nullptr) {
            continue;
        }
        std::shared_ptr<TransformComponent> wpTrans = e->FindComponentOfType<TransformComponent>().lock();
        if (wpTrans == nullptr) {
            continue;
        }
        Mat4 wpMeshTrans = wpTrans->GetWorldMat4();
        wpMeshTrans.ScaleUniform(0.25f);
        _g->_scene->DrawMesh(_debugMesh, wpMeshTrans, Vec4(1.f, 0.f, 0.f, 1.f));
    }
}

bool WaypointFollowComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _g = &g;
    _t = e.FindComponentOfType<TransformComponent>();
    if (_t.expired()) {
        return false;
    }
    _waypointIds.clear();
    _waypointIds.reserve(_waypointNames.size());
    for (std::string const& wpName : _waypointNames) {
        EntityId waypointId = g._entityManager->FindActiveEntityByName(wpName.c_str());
        if (!waypointId.IsValid()) {
            // NOTE: we don't return false (failure) because if we do,
            // EntityEditingContext will delete and reconstruct this component,
            // causing it to lose its _waypointNames (which is bad if we're in
            // the middle of typing an entity)
            // LOL is this actually true? I'm not sure.
            continue;
        }        
        _waypointIds.push_back(waypointId);
    }
    _debugMesh = _g->_scene->GetMesh("cube");
    assert(_debugMesh != nullptr);

    if (_autoStart) {
        _running = true;
    } else {
        _running = false;
    }
    return true;
}

bool WaypointFollowComponent::DrawImGui() {
    bool needReconnect = false;
    ImGui::Checkbox("Auto Start##", &_autoStart);
    ImGui::Checkbox("Loop##", &_loop);
    ImGui::InputScalar("Speed##", ImGuiDataType_Float, &_speed);
    if (ImGui::Button("Add Waypoint##")) {
        _waypointNames.push_back("change_me");
        needReconnect = true;
    }
    char buf[128];
    for (int i = 0; i < _waypointNames.size(); ++i) {
        ImGui::PushID(i);
        std::string& wpName = _waypointNames[i];        
        assert(wpName.length() < 128);
        strcpy(buf, wpName.c_str());
        bool changed = ImGui::InputText("Waypoint name##", buf, 128);
        if (changed) {
            wpName = buf;
            needReconnect = true;
        }
        ImGui::PopID();
    }
    return needReconnect;
}

void WaypointFollowComponent::Save(boost::property_tree::ptree& pt) const {
    pt.put("auto_start", _autoStart);
    pt.put("loop", _loop);
    pt.put("speed", _speed);
    ptree& waypointsPt = pt.add_child("waypoints", ptree());
    for (std::string const& wpName : _waypointNames) {
        ptree& wpPt = waypointsPt.add_child("waypoint_name", ptree());
        wpPt.put_value(wpName);
    }
}

void WaypointFollowComponent::Save(serial::Ptree pt) const {
    pt.PutBool("auto_start", _autoStart);
    pt.PutBool("loop", _loop);
    pt.PutFloat("speed", _speed);
    serial::Ptree waypointsPt = pt.AddChild("waypoints");
    for (std::string const& wpName : _waypointNames) {
        waypointsPt.PutString("waypoint_name", wpName.c_str());
    }
}

void WaypointFollowComponent::Load(boost::property_tree::ptree const& pt) {
    auto const& maybeAutoStart = pt.get_optional<bool>("auto_start");
    if (maybeAutoStart.has_value()) {
        _autoStart = pt.get<bool>("auto_start");
    } else {
        _autoStart = true;
    }
    _loop = pt.get<bool>("loop");
    _speed = pt.get<float>("speed");
    _waypointNames.clear();
    ptree const& waypointsPt = pt.get_child("waypoints");
    for (auto const& wpPt : waypointsPt) {
        _waypointNames.push_back(wpPt.second.get_value<std::string>());
    }
}