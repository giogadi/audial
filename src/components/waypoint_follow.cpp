#include "waypoint_follow.h"

#include "imgui/imgui.h"

#include "game_manager.h"
#include "entity_manager.h"
#include "components/transform.h"
#include "renderer.h"

void WaypointFollowComponent::Update(float dt) {
    if (_currentWaypointIx >= _waypointIds.size()) {
        // not looping for now
        return;
    }
    float constexpr kSpeed = 2.f;
    EntityId wpId = _waypointIds[_currentWaypointIx];
    Entity* wp = _g->_entityManager->GetEntity(wpId);
    assert(wp != nullptr);
    std::shared_ptr<TransformComponent> wpTrans = wp->FindComponentOfType<TransformComponent>().lock();
    Vec3 wpPos = wpTrans->GetWorldPos();
    
    std::shared_ptr<TransformComponent> myTrans = _t.lock();
    Vec3 myPos = myTrans->GetWorldPos();
    Vec3 fromMeToWaypoint = wpPos - myPos;
    

    float distToWp = fromMeToWaypoint.Length();
    float stepSize = kSpeed * dt;
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

void WaypointFollowComponent::EditUpdate(float dt) {
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
        renderer::ColorModelInstance* m = _g->_scene->GetColorModelInstance(_wpModelIds[i]);
        assert(m != nullptr);
        m->_transform = wpTrans->GetWorldMat4();
    }
}

bool WaypointFollowComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _g = &g;
    _waypointIds.clear();
    _waypointIds.reserve(_waypointNames.size());
    _wpModelIds.clear();
    _wpModelIds.reserve(_waypointNames.size());
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
        
        // Add model for this waypoint to renderer.
        std::pair<VersionId, renderer::ColorModelInstance*> item = _g->_scene->AddColorModelInstance();
        _wpModelIds.push_back(item.first);
    }
    return true;
}

void WaypointFollowComponent::EditDestroy() {
    // disconnect wpModels from renderer
    for (VersionId modelId : _wpModelIds) {
        _g->_scene->RemoveColorModelInstance(modelId);
    }
}

bool WaypointFollowComponent::DrawImGui() {
    bool needReconnect = false;
    if (ImGui::Button("Add Waypoint##")) {
        _waypointNames.push_back("");
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
    ptree& waypointsPt = pt.add_child("waypoints", ptree());
    for (std::string const& wpName : _waypointNames) {
        waypointsPt.put("waypoint_name", wpName);
    }
}

void WaypointFollowComponent::Load(boost::property_tree::ptree const& pt) {
    _waypointNames.clear();
    ptree const& waypointsPt = pt.get_child("waypoints");
    for (auto const& wpPt : waypointsPt) {
        _waypointNames.push_back(wpPt.second.get_value<std::string>());
    }
}