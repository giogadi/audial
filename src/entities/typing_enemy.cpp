#include "typing_enemy.h"

#include <sstream>

#include "imgui/imgui.h"

#include "game_manager.h"
#include "renderer.h"
#include "audio.h"
#include "step_sequencer.h"
#include "typing_player.h"
#include "imgui_util.h"
#include "rng.h"
#include "serial_vector_util.h"
#include "math_util.h"

namespace {

void ProjectWorldPointToScreenSpace(Vec3 const& worldPos, Mat4 const& viewProjMatrix, int screenWidth, int screenHeight, float& screenX, float& screenY) {
    Vec4 pos(worldPos._x, worldPos._y, worldPos._z, 1.f);
    pos = viewProjMatrix * pos;
    pos.Set(pos._x / pos._w, pos._y / pos._w, pos._z / pos._w, 1.f);
    // map [-1,1] x [-1,1] to [0,sw] x [0,sh]
    screenX = (pos._x * 0.5f + 0.5f) * screenWidth;
    screenY = (-pos._y * 0.5f + 0.5f) * screenHeight;
}

}

void TypingEnemyEntity::Init(GameManager& g) {
    ne::Entity::Init(g);
    _currentColor = _modelColor;
    if (_sectionId >= 0) {
        int numEntities = 0;
        ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::TypingPlayer, &numEntities);
        assert(numEntities == 1);
        TypingPlayerEntity* player = static_cast<TypingPlayerEntity*>(eIter.GetEntity());
        assert(player != nullptr);
        player->RegisterSectionEnemy(_sectionId, _id);
    }
    _prevWaypointPos = _transform.GetPos();
    // TODO I'M SORRY MOTHER. We need to support the ability for now of
    // spawn_enemy.cpp to init an enemy by setting _hitActions directly, and not
    // having it overwritten.
    if (_useHitActionsOnInitHack) {
        assert(_hitActionStrings.empty());
        for (auto const& pAction : _hitActions) {
            pAction->InitBase(g);
        }
    } else {
        _hitActions.clear();
    }
    // END UGLY HACK
    _hitActions.reserve(_hitActionStrings.size());
    SeqAction::LoadInputs loadInputs;  // default
    for (std::string const& actionStr : _hitActionStrings) {
        std::stringstream ss(actionStr);
        std::unique_ptr<SeqAction> pAction = SeqAction::LoadAction(loadInputs, ss);
        if (pAction != nullptr) {
            pAction->InitBase(g);
            _hitActions.push_back(std::move(pAction));   
        }
    }

    _currentWaypointIx = 0;
    _currentWaypointTime = 0.f;
    _followingWaypoints = _autoStartFollowingWaypoints;
}

void TypingEnemyEntity::Update(GameManager& g, float dt) {
    if (!IsActive(g)) {
        return;
    }

    if (!g._editMode) {
        if (_followingWaypoints && !_waypoints.empty()) {
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
            Vec3 newPos = _prevWaypointPos + lerpFactor * (currentWp._p - _prevWaypointPos);
            _transform.SetTranslation(newPos);
        } else {
            Vec3 p  = _transform.GetPos();
    
            Vec3 dp = _velocity * dt;
            p += dp;
            _transform.SetTranslation(p);
        }       

        if (_destroyIfOffscreenLeft) {
            Vec3 p = _transform.GetPos();
            float constexpr kMinX = -10.f;
            if (p._x < kMinX) {
                g._neEntityManager->TagForDestroy(_id);
            }
        }
    }

    float screenX, screenY;
    Mat4 viewProjTransform = g._scene->GetViewProjTransform();
    ProjectWorldPointToScreenSpace(_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);

    screenX = std::round(screenX);
    screenY = std::round(screenY);
    float constexpr kTextSize = 0.75f;
    if (_numHits > 0) {
        std::string_view substr = std::string_view(_text).substr(0, _numHits);
        g._scene->DrawText(substr, screenX, screenY, /*scale=*/kTextSize, Vec4(1.f,1.f,0.f,1.f));
    }
    if (_numHits < _text.length()) {
        std::string_view substr = std::string_view(_text).substr(_numHits);
        g._scene->DrawText(substr, screenX, screenY, /*scale=*/kTextSize, _textColor);
    }

    if (_model != nullptr) {
        g._scene->DrawMesh(_model, _transform, _currentColor);
    }
}

bool TypingEnemyEntity::IsActive(GameManager& g) const {
    if (g._editMode) {
        return true;
    }
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (_activeBeatTime >= 0.0 && beatTime < _activeBeatTime) {
        return false;
    }
    if (_inactiveBeatTime >= 0.0 && beatTime > _inactiveBeatTime) {
        return false;
    }
    return true;
}

void TypingEnemyEntity::OnHit(GameManager& g) {
    ++_numHits;
    if (_numHits == _text.length()) {
        if (_destroyAfterTyped) {
            bool success = g._neEntityManager->TagForDestroy(_id);
            assert(success);
        }            
    }

    DoHitActions(g);
}

void TypingEnemyEntity::DoHitActions(GameManager& g) {
    if (_hitActions.empty()) {
        return;
    }
    switch (_hitBehavior) {
        case HitBehavior::SingleAction: {
            int hitActionIx = (_numHits - 1) % _hitActions.size();
            SeqAction& a = *_hitActions[hitActionIx];

            a.ExecuteBase(g);
            break;
        }
        case HitBehavior::AllActions: {
            for (auto const& pAction : _hitActions) {
                pAction->ExecuteBase(g);
            }
            break;
        }
    }
}

InputManager::Key TypingEnemyEntity::GetNextKey() const {
    int textIndex = std::min(_numHits, (int) _text.length() - 1);
    char nextChar = std::tolower(_text.at(textIndex));
    int charIx = nextChar - 'a';
    if (charIx < 0 || charIx > static_cast<int>(InputManager::Key::Z)) {
        printf("WARNING: char \'%c\' not in InputManager!\n", nextChar);
        return InputManager::Key::NumKeys;
    }
    InputManager::Key nextKey = static_cast<InputManager::Key>(charIx);
    return nextKey;
}

void TypingEnemyEntity::Waypoint::Save(serial::Ptree pt) const {
    pt.PutFloat("t", _t);
    serial::SaveInNewChildOf(pt, "p", _p);
}

void TypingEnemyEntity::Waypoint::Load(serial::Ptree pt) {
    _t = pt.GetFloat("t");
    if (!serial::LoadFromChildOf(pt, "p", _p)) {
        printf("TypingEnemyEntity::Waypoint::Load: ERROR couldn't find child \"p\"\n");
    }
}

void TypingEnemyEntity::LoadDerived(serial::Ptree pt) {
    _text = pt.GetString("text");
    pt.TryGetBool("type_kill", &_destroyAfterTyped);
    bool allActionsOnHit = false;
    pt.TryGetBool("all_actions_on_hit", &allActionsOnHit);
    if (allActionsOnHit) {
        _hitBehavior = HitBehavior::AllActions;
    } else {
        _hitBehavior = HitBehavior::SingleAction;
    }
    _flowPolarity = false;
    pt.TryGetBool("flow_polarity", &_flowPolarity);
    _flowSectionId = -1;
    pt.TryGetInt("flow_section_id", &_flowSectionId);
    _autoStartFollowingWaypoints = false;
    pt.TryGetBool("waypoint_auto_start", &_autoStartFollowingWaypoints);
    _loopWaypoints = false;
    pt.TryGetBool("loop_waypoints", &_loopWaypoints);
    serial::LoadVectorFromChildNode(pt, "waypoints", _waypoints);
    serial::Ptree actionsPt = pt.GetChild("hit_actions");
    int numChildren;
    serial::NameTreePair* children = actionsPt.GetChildren(&numChildren);
    _hitActionStrings.clear();
    _hitActionStrings.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        _hitActionStrings.push_back(children[i]._pt.GetStringValue());
    }
    delete[] children;
    
}

void TypingEnemyEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutString("text", _text.c_str());
    pt.PutBool("type_kill", _destroyAfterTyped);
    pt.PutBool("all_actions_on_hit", _hitBehavior == HitBehavior::AllActions);
    pt.PutBool("flow_polarity", _flowPolarity);
    pt.PutInt("flow_section_id", _flowSectionId);
    pt.PutBool("waypoint_auto_start", _autoStartFollowingWaypoints);
    pt.PutBool("loop_waypoints", _loopWaypoints);
    serial::SaveVectorInChildNode(pt, "waypoints", "waypoint", _waypoints);
    serial::Ptree actionsPt = pt.AddChild("hit_actions");
    for (std::string const& actionStr : _hitActionStrings) {
        serial::Ptree actionPt = actionsPt.AddChild("action");
        actionPt.PutStringValue(actionStr.c_str());
    }
}

void TypingEnemyEntity::Waypoint::ImGui(GameManager& g) {
    ImGui::InputFloat("Time", &_t);
    ImGui::InputFloat3("Pos", _p._data);
}

ne::BaseEntity::ImGuiResult TypingEnemyEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;

    imgui_util::InputText<32>("Text", &_text);
    ImGui::Checkbox("Kill on type", &_destroyAfterTyped);

    bool allActionsOnHit = _hitBehavior == HitBehavior::AllActions;
    ImGui::Checkbox("All actions on hit", &allActionsOnHit);
    if (allActionsOnHit) {
        _hitBehavior = HitBehavior::AllActions;
    } else {
        _hitBehavior = HitBehavior::SingleAction;
    }

    ImGui::Checkbox("Flow polarity", &_flowPolarity);

    ImGui::InputInt("Section ID", &_flowSectionId);
    
    if (ImGui::Button("Add Action")) {
        _hitActionStrings.emplace_back();
    }
    int deletedIx = -1;
    ImGui::PushID("actions");
    for (int i = 0; i < _hitActionStrings.size(); ++i) {
        ImGui::PushID(i);
        std::string& actionStr = _hitActionStrings[i];
        bool changed = imgui_util::InputText<256>("Action", &actionStr, /*trueOnReturnOnly=*/true);
        if (changed) {
            result = ImGuiResult::NeedsInit;
        }
        if (ImGui::Button("Delete Action")) {
            deletedIx = i;
        }
        ImGui::PopID();
    }
    ImGui::PopID();

    if (deletedIx >= 0) {
        result = ImGuiResult::NeedsInit;
        _hitActionStrings.erase(_hitActionStrings.begin() + deletedIx);
    }

    ImGui::Checkbox("Waypoint Auto Start", &_autoStartFollowingWaypoints);
    ImGui::Checkbox("Loop Waypoints", &_loopWaypoints);

    if (ImGui::Button("Add Waypoint")) {
        _waypoints.emplace_back();
    }
    deletedIx = -1;
    ImGui::PushID("waypoints");
    for (int i = 0; i < _waypoints.size(); ++i) {
        ImGui::PushID(i);
        _waypoints[i].ImGui(g);
        if (ImGui::Button("Delete Waypoint")) {
            deletedIx = i;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    if (deletedIx >= 0) {
        result = ImGuiResult::NeedsInit;
        _waypoints.erase(_waypoints.begin() + deletedIx);
    }
             
    return result;
}

void TypingEnemyEntity::OnEditPick(GameManager& g) {
    DoHitActions(g);
}

static TypingEnemyEntity sMultiEnemy;

void TypingEnemyEntity::MultiSelectImGui(GameManager& g, std::vector<TypingEnemyEntity*>& enemies) {

    ImGui::ColorEdit4("Model color", sMultiEnemy._modelColor._data);
    ImGui::InputInt("Section ID", &sMultiEnemy._flowSectionId);
    
    if (ImGui::Button("Add Action")) {
        sMultiEnemy._hitActionStrings.emplace_back();
    }
    int deletedIx = -1;
    for (int i = 0; i < sMultiEnemy._hitActionStrings.size(); ++i) {
        ImGui::PushID(i);
        std::string& actionStr = sMultiEnemy._hitActionStrings[i];
        imgui_util::InputText<256>("Action", &actionStr);
        if (ImGui::Button("Delete")) {
            deletedIx = i;
        }
        ImGui::PopID();
    }

    if (deletedIx >= 0) {
        sMultiEnemy._hitActionStrings.erase(sMultiEnemy._hitActionStrings.begin() + deletedIx);
    }

    static bool sApplyColor = true;
    static bool sApplySectionId = true;
    static bool sApplyActions = true;    
    ImGui::Checkbox("Apply color", &sApplyColor);
    ImGui::Checkbox("Apply section ID", &sApplySectionId);
    ImGui::Checkbox("Apply actions", &sApplyActions);
    char applySelectionButtonStr[] = "Apply to Selection (x)";
    sprintf(applySelectionButtonStr, "Apply to Selection (%zu)", enemies.size());
    if (ImGui::Button(applySelectionButtonStr)) {
        for (TypingEnemyEntity* e : enemies) {
            if (sApplyColor) {
                e->_modelColor = sMultiEnemy._modelColor;
            }
            if (sApplySectionId) {
                e->_flowSectionId = sMultiEnemy._flowSectionId;
            }
            if (sApplyActions) {
                e->_hitActionStrings = sMultiEnemy._hitActionStrings;
            }            
            // TODO: do I need to call Destroy()?
            e->Init(g);
        }        
    }

    if (ImGui::Button("Randomize text")) {
        for (TypingEnemyEntity* e : enemies) {
            int letterIx = rng::GetInt(0, 25);
            char letter = 'a' + letterIx;
            e->_text = std::string(1, letter);
        }
    }
}
