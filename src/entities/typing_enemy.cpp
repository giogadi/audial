#include "typing_enemy.h"

#include <sstream>

#include "imgui/imgui.h"

#include "game_manager.h"
#include "renderer.h"
#include "audio.h"
#include "step_sequencer.h"
#include "imgui_util.h"
#include "rng.h"
#include "serial_vector_util.h"
#include "math_util.h"
#include "editor.h"
#include "geometry.h"
#include "entities/flow_player.h"
#include "string_util.h"

extern GameManager gGameManager;

extern bool gRandomLetters;

namespace {
int gShuffledLetterIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
int gCurrentShuffleIx = 26; // one past

InputManager::ControllerButton CharToButton(char c) {
    switch (c) {
        case 'a': return InputManager::ControllerButton::PadUp;
        case 's': return InputManager::ControllerButton::PadDown;
        case 'd': return InputManager::ControllerButton::PadLeft;
        case 'f': return InputManager::ControllerButton::PadRight;
        case 'h': return InputManager::ControllerButton::ButtonTop;
        case 'j': return InputManager::ControllerButton::ButtonBottom;
        case 'k': return InputManager::ControllerButton::ButtonLeft;
        case 'l': return InputManager::ControllerButton::ButtonRight;            
        case 'e': return InputManager::ControllerButton::BumperLeft;
        case 'r': return InputManager::ControllerButton::TriggerLeft;
        case 'y': return InputManager::ControllerButton::BumperRight;
        case 'u': return InputManager::ControllerButton::TriggerRight;
        default: {
            printf("InputManager::ControllerButton::CharToButton: unrecognized char \'%c\'\n", c);
            return InputManager::ControllerButton::Count;
        }
    }
}

// force b to match a length
void ForceStringsSameLength(std::string const& name, std::string& a, std::string& b) {
    if (a.length() != b.length()) {
        printf("WARNING TypingEnemyEntity \"%s\" has keyText and buttons with different lengths: \"%s\" vs \"%s\"\n", name.c_str(), a.c_str(), b.c_str());
    }
    if (a.length() < b.length()) {
        b.resize(a.length());
    } else if (b.length() < a.length()) {
        for (int i = b.length(); i < a.length(); ++i) {
            b.push_back('a');
        }
    }
}
}

void TypingEnemyEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();

    int constexpr kRefactor231024Version = 1;
    int const scriptVersion = pt.GetVersion();
    if (scriptVersion < kRefactor231024Version) {
        bool flowPolarity = false;
        pt.TryGetBool("flow_polarity", &flowPolarity);
        if (flowPolarity) {
            _p._hitResponseType = HitResponseType::Push;
        } else {
            _p._hitResponseType = HitResponseType::Pull;
        }

    } else {
        if (!serial::TryGetEnum(pt, "hit_response_type", _p._hitResponseType)) {
            // backward compat
            serial::TryGetEnum(pt, "enemy_type", _p._hitResponseType);
        }        
    }

    pt.TryGetBool("use_last_hit_response", &_p._useLastHitResponse);
    serial::TryGetEnum(pt, "last_hit_response_type", _p._lastHitResponseType);
    pt.TryGetFloat("last_hit_vel", &_p._lastHitVelocity);
    pt.TryGetFloat("last_hit_push_angle_deg", &_p._lastHitPushAngleDeg);
    
    _p._keyText = pt.GetString("text");
    pt.TryGetString("buttons", &_p._buttons);
    pt.TryGetBool("type_kill", &_p._destroyAfterTyped);
    bool allActionsOnHit = false;
    if (pt.TryGetBool("all_actions_on_hit", &allActionsOnHit)) {
        if (allActionsOnHit) {
            _p._hitBehavior = HitBehavior::AllActions;
        }
        else {
            _p._hitBehavior = HitBehavior::SingleAction;
        }
    }

    pt.TryGetDouble("flow_cooldown", &_p._flowCooldownBeatTime);
    pt.TryGetBool("show_beats_left", &_p._showBeatsLeft);
    pt.TryGetFloat("cooldown_quantize_denom", &_p._cooldownQuantizeDenom);
    pt.TryGetBool("reset_cooldown_on_any_hit", &_p._resetCooldownOnAnyHit);
    pt.TryGetBool("init_hittable", &_p._initHittable);
    pt.TryGetBool("stop_on_pass", &_p._stopOnPass);
    pt.TryGetFloat("dash_vel", &_p._dashVelocity);
    pt.TryGetFloat("push_angle_deg", &_p._pushAngleDeg);
    serial::LoadFromChildOf(pt, "active_region_editor_id", _p._activeRegionEditorId);

    SeqAction::LoadActionsFromChildNode(pt, "hit_actions", _p._hitActions);
    SeqAction::LoadActionsFromChildNode(pt, "all_hit_actions", _p._allHitActions);
    SeqAction::LoadActionsFromChildNode(pt, "off_cooldown_actions", _p._offCooldownActions);
    SeqAction::LoadActionsFromChildNode(pt, "combo_end_actions", _p._comboEndActions);

    if (gRandomLetters) {
        if (gCurrentShuffleIx > 25) {
            // reshuffle
            for (int i = 0; i < 26; ++i) {
                int swapIx = rng::GetInt(i, 25);
                if (swapIx != i) {
                    std::swap(gShuffledLetterIndices[i], gShuffledLetterIndices[swapIx]);
                }
            }
            gCurrentShuffleIx = 0;
        }
        int randLetterIx = gShuffledLetterIndices[gCurrentShuffleIx];
        ++gCurrentShuffleIx;
        _p._keyText.resize(1);
        _p._keyText[0] = 'a' + randLetterIx;
    }
}

void TypingEnemyEntity::SaveDerived(serial::Ptree pt) const {
    serial::PutEnum(pt, "hit_response_type", _p._hitResponseType);
    pt.PutFloat("dash_vel", _p._dashVelocity);
    pt.PutFloat("push_angle_deg", _p._pushAngleDeg);

    pt.PutBool("use_last_hit_response", _p._useLastHitResponse);
    serial::PutEnum(pt, "last_hit_response_type", _p._lastHitResponseType);
    pt.PutFloat("last_hit_vel", _p._lastHitVelocity);
    pt.PutFloat("last_hit_push_angle_deg", _p._lastHitPushAngleDeg);
    
    pt.PutString("text", _p._keyText.c_str());
    pt.PutString("buttons", _p._buttons.c_str());
    pt.PutBool("type_kill", _p._destroyAfterTyped);
    pt.PutBool("all_actions_on_hit", _p._hitBehavior == HitBehavior::AllActions);
    pt.PutDouble("flow_cooldown", _p._flowCooldownBeatTime);
    pt.PutBool("show_beats_left", _p._showBeatsLeft);
    pt.PutFloat("cooldown_quantize_denom", _p._cooldownQuantizeDenom);
    pt.PutBool("reset_cooldown_on_any_hit", _p._resetCooldownOnAnyHit);
    pt.PutBool("init_hittable", _p._initHittable);
    pt.PutBool("stop_on_pass", _p._stopOnPass);
    
    serial::SaveInNewChildOf(pt, "active_region_editor_id", _p._activeRegionEditorId);
    SeqAction::SaveActionsInChildNode(pt, "hit_actions", _p._hitActions);
    SeqAction::SaveActionsInChildNode(pt, "all_hit_actions", _p._allHitActions);
    SeqAction::SaveActionsInChildNode(pt, "off_cooldown_actions", _p._offCooldownActions);
    SeqAction::SaveActionsInChildNode(pt, "combo_end_actions", _p._comboEndActions);
}

ne::BaseEntity::ImGuiResult TypingEnemyEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;

    if (HitResponseTypeImGui("Hit Response", &_p._hitResponseType)) {
        result = ImGuiResult::NeedsInit;
    }

    ImGui::Checkbox("Has last hit response", &_p._useLastHitResponse);
    if (_p._useLastHitResponse) {
        HitResponseTypeImGui("Last hit response", &_p._lastHitResponseType);
        ImGui::InputFloat("Last vel", &_p._lastHitVelocity);
        ImGui::InputFloat("Last push deg", &_p._lastHitPushAngleDeg);
    }

    if (imgui_util::InputText<32>("Text", &_p._keyText, true)) {
        result = ImGuiResult::NeedsInit;
    }
    if (imgui_util::InputText<32>("Buttons", &_p._buttons, true)) {
        result = ImGuiResult::NeedsInit;
    }
    ImGui::Checkbox("Kill on type", &_p._destroyAfterTyped);

    bool allActionsOnHit = _p._hitBehavior == HitBehavior::AllActions;
    ImGui::Checkbox("All actions on hit", &allActionsOnHit);
    if (allActionsOnHit) {
        _p._hitBehavior = HitBehavior::AllActions;
    }
    else {
        _p._hitBehavior = HitBehavior::SingleAction;
    }
    ImGui::InputDouble("Flow cooldown (beat)", &_p._flowCooldownBeatTime);
    ImGui::InputFloat("Cooldown quantize denom", &_p._cooldownQuantizeDenom);
    ImGui::Checkbox("Show beats left", &_p._showBeatsLeft);
    ImGui::Checkbox("Reset cooldown on any hit", &_p._resetCooldownOnAnyHit);
    ImGui::Checkbox("Init hittable", &_p._initHittable);
    ImGui::Checkbox("Stop on pass", &_p._stopOnPass);
    ImGui::InputFloat("Dash vel", &_p._dashVelocity);
    ImGui::InputFloat("Push angle (deg)", &_p._pushAngleDeg);
    imgui_util::InputEditorId("Active Region", &_p._activeRegionEditorId);
    if (ImGui::Button("Set Region color")) {
        if (ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_p._activeRegionEditorId, ne::EntityType::FlowTrigger)) {
            Vec4 color = _modelColor;
            color._w = 0.2f;
            e->_modelColor = color;
        }
    }
    if (SeqAction::ImGui("Hit actions", _p._hitActions)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("On-All-Hit actions", _p._allHitActions)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("Off-cooldown actions", _p._offCooldownActions)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("Combo end actions", _p._comboEndActions)) {
        result = ImGuiResult::NeedsInit;
    }

    return result;
}

void TypingEnemyEntity::InitDerived(GameManager& g) {
    _s = State();
    _s._hittable = _p._initHittable;
    _s._currentColor = _modelColor;

    for (auto const& pAction : _p._hitActions) {
        pAction->Init(g);
    }
    for (auto const& pAction : _p._allHitActions) {
        pAction->Init(g);
    }
    for (auto const& pAction : _p._offCooldownActions) {
        pAction->Init(g);
    }
    for (auto const& pAction : _p._comboEndActions) {
        pAction->Init(g);
    }

    ForceStringsSameLength(_name, _p._keyText, _p._buttons);

    if (ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_p._activeRegionEditorId, ne::EntityType::FlowTrigger)) {
        _s._activeRegionId = e->_id;
    }
}

namespace {
bool PlayerWithinRadius(FlowPlayerEntity const& player, TypingEnemyEntity const& enemy, GameManager& g) {
    if (!enemy._s._activeRegionId.IsValid()) {
        return true;
    }
    if (ne::Entity* region = g._neEntityManager->GetEntity(enemy._s._activeRegionId)) {
        bool inside = geometry::IsPointInBoundingBox(player._transform.Pos(), region->_transform);
        return inside;
    }
    return true;
}

void DrawPullEnemy(float const dt, GameManager& g, TypingEnemyEntity& e, FlowPlayerEntity const* player) {
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    
    bool playerWithinRadius = true;
    // TODO: we're computing twice per frame whether enemies are within this radius. BORING    
    if (player) {            
        playerWithinRadius = PlayerWithinRadius(*player, e, g);
    }

    bool showControllerInputs;
    if (g._editMode) {
        showControllerInputs = g._editor->_showControllerInputs;
    } else {
        showControllerInputs = g._inputManager->IsUsingController();
    }

    Transform renderTrans = e._transform;
    // Apply bump logic
    if (e._s._bumpTimer >= 0.f) {
        float constexpr kTotalBumpTime = 0.1f;
        float constexpr kBumpAmount = 0.15f;
        float factor = math_util::Clamp(e._s._bumpTimer / kTotalBumpTime, 0.f, 1.f);
        factor = math_util::Triangle(factor);
        float dist = factor * kBumpAmount;
        Vec3 p = renderTrans.Pos();
        p += e._s._bumpDir * dist;
        renderTrans.SetPos(p);
        e._s._bumpTimer += dt;
        if (e._s._bumpTimer > kTotalBumpTime) {
            e._s._bumpTimer = -1.f;
        }
    }

    //
    // TEXT DRAWING
    //
    {
        int numFadedButtons = 0;
        if (e._s._flowCooldownStartBeatTime > 0.f || !e._s._hittable || !playerWithinRadius) {
            numFadedButtons = e._p._keyText.length();
        } else {
            numFadedButtons = e._s._numHits;
        }
        float constexpr kFadeAlpha = 0.2f;
        if (showControllerInputs) {
            Transform t = renderTrans;
            float const size = 0.25f;
            t.SetScale(Vec3(size, size, size));
            t.SetTranslation(renderTrans.Pos() + Vec3(-size, 0.f, size));
            for (int i = 0; i < e._p._buttons.length(); ++i) {
                InputManager::ControllerButton b = CharToButton(e._p._buttons[i]);
                renderer::ModelInstance* m = g._scene->DrawPsButton(b, t.Mat4Scale());
                m->_topLayer = true;
                if (i < numFadedButtons) {
                    m->_color._w = kFadeAlpha;
                }
                Vec3 p = t.GetPos();
                p._x += 2 * size;
                t.SetTranslation(p);
            }
        } else {
            float constexpr kTextSize = 1.5f;
            std::string_view fadedText = std::string_view(e._p._keyText).substr(0, numFadedButtons);
            Vec4 color(1.f, 1.f, 1.f, kFadeAlpha);
            if (!fadedText.empty()) {
                // AHHHH ALLOCATION
                g._scene->DrawTextWorld(std::string(fadedText), renderTrans.GetPos(), kTextSize, color);
            }
            color._w = 1.f;
            std::string_view activeText = std::string_view(e._p._keyText).substr(numFadedButtons);
            if (!activeText.empty()) {
                bool appendToPrevious = numFadedButtons > 0;
                // AHHHH ALLOCATION
                g._scene->DrawTextWorld(std::string(activeText), renderTrans.GetPos(), kTextSize, color, appendToPrevious);
            }
        }
    }
    
    int constexpr kNumStepsX = 2;
    int constexpr kNumStepsZ = 2;
    float constexpr xStep = 1.f / (float) kNumStepsX;
    float constexpr zStep = 1.f / (float) kNumStepsZ;
    Mat4 localToWorld = renderTrans.Mat4Scale();
    Mat4 subdivMat = localToWorld;
    // subdivMat.Scale(xStep * 0.8f, 1.f, zStep * 0.8f);
    subdivMat.Scale(xStep * 0.8f, xStep * 0.8f, zStep * 0.8f);
    Vec4 color = e._s._currentColor;
    // We adjust the scale of localToWorld AFTER setting subdivMat so that
    // changing the scale of localToWorld doesn't make the subdiv dudes
    // bigger
    if (e._s._flowCooldownStartBeatTime > 0.f || !e._s._hittable || !playerWithinRadius) {
        color._x *= 0.5f;
        color._y *= 0.5f;
        color._z *= 0.5f;
        // color._w = 0.5f;
    }
    if (e._s._flowCooldownStartBeatTime > 0.f /* || !_hittable*/) {
        // timeLeft: [0, flowCooldown] --> [1, explodeScale]
        float constexpr kExplodeMaxScale = 2.f;
        double const cooldownTimeElapsed = math_util::Clamp(beatTime - e._s._flowCooldownStartBeatTime, 0.0, e._p._flowCooldownBeatTime);
        float factor = 1.f;
        if (e._s._flowCooldownStartBeatTime > 0.f) {
            factor = 1.f - static_cast<float>(cooldownTimeElapsed / e._p._flowCooldownBeatTime);
            factor = math_util::SmoothStep(factor);
        }
        float explodeScale = 1.f + factor * (kExplodeMaxScale - 1.f);
        localToWorld.Scale(explodeScale, 1.f, explodeScale);
        if (e._p._showBeatsLeft) {
            float constexpr kBeatCellSize = 0.25f;
            float constexpr kSpacing = 0.1f;
            float constexpr kRowSpacing = 0.1f;
            float constexpr kRowLength = 4.f * kBeatCellSize + (3.f) * kSpacing;
            float const totalBeats = std::ceil(e._p._flowCooldownBeatTime);
            int const numRows = ((static_cast<int>(totalBeats) - 1) / 4) + 1;
            float const vertSize = numRows * kBeatCellSize + (numRows - 1) * kRowSpacing;
            float const rowStartXPos = renderTrans.GetPos()._x - 0.5f * kRowLength;
            float const rowStartZPos = renderTrans.GetPos()._z - 0.5f - vertSize;
            Vec3 firstBeatPos = renderTrans.GetPos() + Vec3(rowStartXPos, 0.f, rowStartZPos);
            Mat4 m;
            m.SetTranslation(firstBeatPos);
            m.ScaleUniform(kBeatCellSize);
            int const numLeft = static_cast<int>(std::ceil(e._p._flowCooldownBeatTime - cooldownTimeElapsed));
            for (int row = 0, beatIx = 0; row < numRows; ++row) {
                for (int col = 0; col < 4 && beatIx < numLeft; ++col, ++beatIx) {
                    Vec3 p(rowStartXPos + col * (kSpacing + kBeatCellSize), 0.f, rowStartZPos + row * (kRowSpacing + kBeatCellSize));
                    m.SetTranslation(p);
                    g._scene->DrawCube(m, Vec4(0.f, 1.f, 0.f, 1.f));
                }
            }
        }
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
}

void DrawPushEnemy(float const dt, GameManager& g, TypingEnemyEntity& e, FlowPlayerEntity const* player) {
    DrawPullEnemy(dt, g, e, player);
}
}

void TypingEnemyEntity::UpdateDerived(GameManager& g, float dt) {      
    if (!g._editMode) {
        Vec3 p  = _transform.GetPos();
    
        Vec3 dp = _s._velocity * dt;
        p += dp;
        _transform.SetTranslation(p);        
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    if (_s._flowCooldownStartBeatTime > 0.f) {
        double const cooldownFinishBeatTime = _s._flowCooldownStartBeatTime + _p._flowCooldownBeatTime;
        if (beatTime >= cooldownFinishBeatTime) {
            for (auto& pAction : _p._offCooldownActions) {
                pAction->Execute(g);
            }
            _s._flowCooldownStartBeatTime = -1.0;
            _s._numHits = 0;
            _s._timeOfLastHit = -1.0;
        }
    }    
}

void TypingEnemyEntity::Draw(GameManager& g, float dt) {

    FlowPlayerEntity* player = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));

    switch (_p._hitResponseType) {
        case HitResponseType::None:
            DrawPullEnemy(dt, g, *this, player);
            break;
        case HitResponseType::Pull:
            DrawPullEnemy(dt, g, *this, player);
            break;
        case HitResponseType::Push:
            DrawPushEnemy(dt, g, *this, player);
            break;
        case HitResponseType::Count:
            assert(false);
            break;
    }    
}

TypingEnemyEntity::HitResponse TypingEnemyEntity::OnHit(GameManager& g) {
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    ++_s._numHits;
    if (_s._numHits == _p._keyText.length()) {
        for (auto const& pAction : _p._allHitActions) {
            pAction->Execute(g);
        }
    }
    if (_s._numHits >= _p._keyText.length()) {
        _s._numHits = _p._keyText.length();
        if (_p._cooldownQuantizeDenom > 0.f) {
            _s._flowCooldownStartBeatTime = BeatClock::GetNextBeatDenomTime(beatTime, _p._cooldownQuantizeDenom);
        } else {
            _s._flowCooldownStartBeatTime = beatTime;
        }
        if (_p._destroyAfterTyped) {
            bool success = g._neEntityManager->TagForDeactivate(_id);
            assert(success);
        }
    }
    _s._timeOfLastHit = beatTime;

    DoHitActions(g);

    HitResponse response;
    if (_p._useLastHitResponse && _s._numHits >= _p._keyText.length()) {
        response._type = _p._lastHitResponseType;
        response._dashSpeed = _p._lastHitVelocity;
    } else {
        response._type = _p._hitResponseType;
        response._dashSpeed = _p._dashVelocity;;
    }  
    return response;
}

void TypingEnemyEntity::DoHitActions(GameManager& g) {
    if (_p._hitActions.empty()) {
        return;
    }
    switch (_p._hitBehavior) {
        case HitBehavior::SingleAction: {
            int hitActionIx = (_s._numHits - 1) % _p._hitActions.size();
            SeqAction& a = *_p._hitActions[hitActionIx];

            a.Execute(g);
            break;
        }
        case HitBehavior::AllActions: {
            for (auto const& pAction : _p._hitActions) {
                pAction->Execute(g);
            }
            break;
        }
    }
}

void TypingEnemyEntity::DoComboEndActions(GameManager& g) {
    for (auto const& pAction : _p._comboEndActions) {
        pAction->Execute(g);
    }
}

void TypingEnemyEntity::OnHitOther(GameManager& g) {
    if (_p._resetCooldownOnAnyHit) {
        _s._flowCooldownStartBeatTime = -1.0;
        _s._numHits = 0;
        _s._timeOfLastHit = -1.0;
    }
}

InputManager::Key TypingEnemyEntity::GetNextKey() const {
    int textIndex = std::min(_s._numHits, (int) _p._keyText.length() - 1);
    char nextChar = std::tolower(_p._keyText.at(textIndex));
    int charIx = nextChar - 'a';
    if (charIx < 0 || charIx > static_cast<int>(InputManager::Key::Z)) {
        printf("WARNING: char \'%c\' not in InputManager!\n", nextChar);
        return InputManager::Key::NumKeys;
    }
    InputManager::Key nextKey = static_cast<InputManager::Key>(charIx);
    return nextKey;
}

InputManager::ControllerButton TypingEnemyEntity::GetNextButton() const {
    int textIndex = std::min(_s._numHits, (int)_p._buttons.length() - 1);
    char nextChar = std::tolower(_p._buttons.at(textIndex));
    return CharToButton(nextChar);
}

void TypingEnemyEntity::OnEditPick(GameManager& g) {
    DoHitActions(g);
}

bool TypingEnemyEntity::CanHit(FlowPlayerEntity const& player, GameManager& g) const {
    if (!PlayerWithinRadius(player, *this, g)) {
        return false;
    }
    if (!_s._hittable) {
        return false;
    }
    return _s._flowCooldownStartBeatTime <= 0.0;
}

static TypingEnemyEntity sMultiEnemy;

void TypingEnemyEntity::MultiSelectImGui(GameManager& g, std::vector<TypingEnemyEntity*>& enemies) {

    ImGui::InputInt("Entity tag", &sMultiEnemy._tag);
    imgui_util::ColorEdit4("Model color", &sMultiEnemy._modelColor);
    ImGui::InputInt("Section ID", &sMultiEnemy._flowSectionId);

    static bool sApplyTag = false;
    static bool sApplyColor = false;
    static bool sApplySectionId = false;
    static bool sApplyActions = false;
    ImGui::Checkbox("Apply tag", &sApplyTag);
    ImGui::Checkbox("Apply color", &sApplyColor);
    ImGui::Checkbox("Apply section ID", &sApplySectionId);
    ImGui::Checkbox("Apply actions", &sApplyActions);
    char applySelectionButtonStr[] = "Apply to Selection (xxxx)";
    sprintf(applySelectionButtonStr, "Apply to Selection (%zu)", enemies.size());
    if (ImGui::Button(applySelectionButtonStr)) {
        for (TypingEnemyEntity* e : enemies) {
            if (sApplyTag) {
                e->_tag = sMultiEnemy._tag;
            }
            if (sApplyColor) {
                e->_modelColor = sMultiEnemy._modelColor;
            }
            if (sApplySectionId) {
                e->_flowSectionId = sMultiEnemy._flowSectionId;
            }
            if (sApplyActions) {
                // e->_hitActionStrings = sMultiEnemy._hitActionStrings;
            }            
            // TODO: do I need to call Destroy()?
            e->Init(g);
        }        
    }

    if (ImGui::Button("Randomize text")) {
        for (TypingEnemyEntity* e : enemies) {
            int letterIx = rng::GetInt(0, 25);
            char letter = 'a' + letterIx;
            e->_p._keyText = std::string(1, letter);
        }
    }
}

void TypingEnemyEntity::SetHittable(bool hittable) {
    _s._hittable = hittable;
}

void TypingEnemyEntity::Bump(Vec3 const& bumpDir) {
    _s._bumpTimer = 0.f;
    _s._bumpDir = bumpDir;
}
