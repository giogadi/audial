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
#include "editor.h"
#include "geometry.h"

void TypingEnemyEntity::InitDerived(GameManager& g) {
    _currentColor = _modelColor;
    if (_typingSectionId >= 0) {
        int numEntities = 0;
        ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::TypingPlayer, &numEntities);
        assert(numEntities == 1);
        TypingPlayerEntity* player = static_cast<TypingPlayerEntity*>(eIter.GetEntity());
        assert(player != nullptr);
        player->RegisterSectionEnemy(_typingSectionId, _id);
    }
    
    _waypointFollower.Init(g, *this);
        
    // TODO I'M SORRY MOTHER. We need to support the ability for now of
    // spawn_enemy.cpp to init an enemy by setting _hitActions directly, and not
    // having it overwritten.
    if (_useHitActionsOnInitHack) {
        assert(_hitActionStrings.empty());
        for (auto const& pAction : _hitActions) {
            pAction->Init(g);
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
            pAction->Init(g);
            _hitActions.push_back(std::move(pAction));   
        }
    }

    _flowCooldownTimeLeft = -1.f;
}

void TypingEnemyEntity::Update(GameManager& g, float dt) {    
    if (g._editMode && g._editor->IsEntitySelected(_id)) {
        _waypointFollower.Update(g, dt, /*editModeSelected=*/true, this);
    }
    
    if (!IsActive(g)) {
        return;
    }

    if (!g._editMode) {
        bool followsWaypoint = _waypointFollower.Update(g, dt, /*editModeSelected=*/false, this);
        if (!followsWaypoint) {
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
    geometry::ProjectWorldPointToScreenSpace(_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);

    screenX = std::round(screenX);
    screenY = std::round(screenY);
    float constexpr kTextSize = 0.75f;
    if (_flowCooldownTimeLeft > 0.f) {
        Vec4 color = _textColor;
        color._w = 0.2f;
        g._scene->DrawText(std::string_view(_text), screenX, screenY, kTextSize, color);
    } else if (_text.length() > 1) {
        if (_numHits > 0) {
            std::string_view substr = std::string_view(_text).substr(0, _numHits);
            g._scene->DrawText(substr, screenX, screenY, /*scale=*/kTextSize, Vec4(1.f,1.f,0.f,1.f));
        }
        if (_numHits < _text.length()) {
            std::string_view substr = std::string_view(_text).substr(_numHits);
            g._scene->DrawText(substr, screenX, screenY, /*scale=*/kTextSize, _textColor);
        }
    } else {
        g._scene->DrawText(std::string_view(_text), screenX, screenY, /*scale=*/kTextSize, _textColor);
    }

    // Maybe update color from getting hit
    // turn to FadeColor on hit, then fade back to regular color
    if (_timeOfLastHit >= 0.0) {
        double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double constexpr kFadeTime = 0.25;
        double fadeFactor = (beatTime - _timeOfLastHit) / kFadeTime;        
        fadeFactor = math_util::Clamp(fadeFactor, 0.0, 1.0);
        Vec4 constexpr kFadeColor(0.f, 0.f, 0.f, 1.f);
        if (fadeFactor == 1.0) {
            _timeOfLastHit = -1.0;
        }
        _currentColor = kFadeColor + fadeFactor * (_modelColor - kFadeColor);
    }

    if (_flowCooldownTimeLeft > 0.f) {
        _flowCooldownTimeLeft -= dt;
    }

    if (_flowCooldown > 0.f) {
        int constexpr kNumStepsX = 2;
        int constexpr kNumStepsZ = 2;
        float constexpr xStep = 1.f / (float) kNumStepsX;
        float constexpr zStep = 1.f / (float) kNumStepsZ;
        Mat4 localToWorld = _transform.Mat4Scale();
        Mat4 subdivMat = localToWorld;
        subdivMat.Scale(xStep * 0.8f, 1.f, zStep * 0.8f);
        Vec4 color = _currentColor;
        // We adjust the scale of localToWorld AFTER setting subdivMat so that
        // changing the scale of localToWorld doesn't make the subdiv dudes
        // bigger
        if (_flowCooldownTimeLeft > 0.f) {
            color._w = 0.5f;
            // timeLeft: [0, flowCooldown] --> [1, explodeScale]
            float constexpr kExplodeMaxScale = 2.f;
            float factor = math_util::Clamp(_flowCooldownTimeLeft / _flowCooldown, 0.f, 1.f);
            factor = math_util::SmoothStep(factor);
            float explodeScale = 1.f + factor * (kExplodeMaxScale - 1.f);
            localToWorld.Scale(explodeScale, 1.f, explodeScale);            
        }
        
        Vec4 localPos(-0.5f + 0.5f * xStep, 0.f, -0.5f + 0.5f * zStep, 1.f);
        for (int z = 0; z < kNumStepsZ; ++z) {
            localPos._x = -0.5f + 0.5f * xStep;
            for (int x = 0; x < kNumStepsX; ++x) {
                Vec4 worldPos = localToWorld * localPos;
                subdivMat.SetCol(3, worldPos);
                g._scene->DrawCube(subdivMat, color);
                localPos._x += xStep;
            }
            localPos._z += zStep;
        }
    } else {
        if (_model != nullptr) {
            g._scene->DrawMesh(_model, _transform.Mat4Scale(), _currentColor);
        }
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
    _flowCooldownTimeLeft = _flowCooldown;
    ++_numHits;
    if (_numHits == _text.length()) {
        if (_destroyAfterTyped) {
            bool success = g._neEntityManager->TagForDestroy(_id);
            assert(success);
        }            
    }
    _timeOfLastHit = g._beatClock->GetBeatTimeFromEpoch();

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

            a.Execute(g);
            break;
        }
        case HitBehavior::AllActions: {
            for (auto const& pAction : _hitActions) {
                pAction->Execute(g);
            }
            break;
        }
    }
}

void TypingEnemyEntity::OnHitOther(GameManager& g) {
    if (_resetCooldownOnAnyHit) {
        _flowCooldownTimeLeft = -1.f;
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

    _flowCooldown = -1.f;
    pt.TryGetFloat("flow_cooldown", &_flowCooldown);

    _resetCooldownOnAnyHit = false;
    pt.TryGetBool("reset_cooldown_on_any_hit", &_resetCooldownOnAnyHit);

    bool hasWaypointFollower = serial::LoadFromChildOf(pt, "waypoint_follower", _waypointFollower);
    if (!hasWaypointFollower) {
        // backward compat
        _waypointFollower.Load(pt);
    }
    
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
    pt.PutFloat("flow_cooldown", _flowCooldown);
    pt.PutBool("reset_cooldown_on_any_hit", _resetCooldownOnAnyHit);
    serial::SaveInNewChildOf(pt, "waypoint_follower", _waypointFollower);

    serial::Ptree actionsPt = pt.AddChild("hit_actions");
    for (std::string const& actionStr : _hitActionStrings) {
        serial::Ptree actionPt = actionsPt.AddChild("action");
        actionPt.PutStringValue(actionStr.c_str());
    }
    // HOWDYYYYYYYYYYY
    SeqAction::SaveActionsInChildNode(pt, "new_hit_actions", _hitActions);
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
    ImGui::InputFloat("Flow cooldown", &_flowCooldown);
    ImGui::Checkbox("Reset cooldown on any hit", &_resetCooldownOnAnyHit);
    
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

    ImGui::PushID("wp");
    _waypointFollower.ImGui();
    ImGui::PopID();    
             
    return result;
}

void TypingEnemyEntity::OnEditPick(GameManager& g) {
    DoHitActions(g);
}

bool TypingEnemyEntity::CanHit() const {
    return _flowCooldownTimeLeft <= 0.f;
}

static TypingEnemyEntity sMultiEnemy;

void TypingEnemyEntity::MultiSelectImGui(GameManager& g, std::vector<TypingEnemyEntity*>& enemies) {

    imgui_util::ColorEdit4("Model color", &sMultiEnemy._modelColor);
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
