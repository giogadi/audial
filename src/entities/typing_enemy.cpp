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
    _keyText = pt.GetString("text");
    pt.TryGetString("buttons", &_buttons);
    pt.TryGetBool("type_kill", &_destroyAfterTyped);
    bool allActionsOnHit = false;
    pt.TryGetBool("all_actions_on_hit", &allActionsOnHit);
    if (allActionsOnHit) {
        _hitBehavior = HitBehavior::AllActions;
    } else {
        _hitBehavior = HitBehavior::SingleAction;
    }
    _bounceRadius = -1.f;
    pt.TryGetFloat("bounce_radius", &_bounceRadius);
    _flowPolarity = false;
    pt.TryGetBool("flow_polarity", &_flowPolarity);

    _flowCooldownBeatTime = -1.0;
    pt.TryGetDouble("flow_cooldown", &_flowCooldownBeatTime);

    _showBeatsLeft = false;
    pt.TryGetBool("show_beats_left", &_showBeatsLeft);

    _cooldownQuantizeDenom = 0.f;
    pt.TryGetFloat("cooldown_quantize_denom", &_cooldownQuantizeDenom);

    _resetCooldownOnAnyHit = false;
    pt.TryGetBool("reset_cooldown_on_any_hit", &_resetCooldownOnAnyHit);

    _initHittable = true;
    pt.TryGetBool("init_hittable", &_initHittable);

    _activeRegionEditorId = EditorId();
    serial::LoadFromChildOf(pt, "active_region_editor_id", _activeRegionEditorId);

    SeqAction::LoadActionsFromChildNode(pt, "hit_actions", _hitActions);
    SeqAction::LoadActionsFromChildNode(pt, "all_hit_actions", _allHitActions);
    SeqAction::LoadActionsFromChildNode(pt, "off_cooldown_actions", _offCooldownActions);

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
        _keyText.resize(1);
        _keyText[0] = 'a' + randLetterIx;
    }
}

void TypingEnemyEntity::SaveDerived(serial::Ptree pt) const {    
    pt.PutString("text", _keyText.c_str());
    pt.PutString("buttons", _buttons.c_str());
    pt.PutBool("type_kill", _destroyAfterTyped);
    pt.PutBool("all_actions_on_hit", _hitBehavior == HitBehavior::AllActions);
    pt.PutFloat("bounce_radius", _bounceRadius);
    pt.PutBool("flow_polarity", _flowPolarity);
    pt.PutDouble("flow_cooldown", _flowCooldownBeatTime);
    pt.PutBool("show_beats_left", _showBeatsLeft);
    pt.PutFloat("cooldown_quantize_denom", _cooldownQuantizeDenom);
    pt.PutBool("reset_cooldown_on_any_hit", _resetCooldownOnAnyHit);
    pt.PutBool("init_hittable", _initHittable);
    serial::SaveInNewChildOf(pt, "active_region_editor_id", _activeRegionEditorId);
    SeqAction::SaveActionsInChildNode(pt, "hit_actions", _hitActions);
    SeqAction::SaveActionsInChildNode(pt, "all_hit_actions", _allHitActions);
    SeqAction::SaveActionsInChildNode(pt, "off_cooldown_actions", _offCooldownActions);
}

ne::BaseEntity::ImGuiResult TypingEnemyEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;

    if (imgui_util::InputText<32>("Text", &_keyText, true)) {
        result = ImGuiResult::NeedsInit;
    }
    if (imgui_util::InputText<32>("Buttons", &_buttons, true)) {
        result = ImGuiResult::NeedsInit;
    }
    ImGui::Checkbox("Kill on type", &_destroyAfterTyped);

    bool allActionsOnHit = _hitBehavior == HitBehavior::AllActions;
    ImGui::Checkbox("All actions on hit", &allActionsOnHit);
    if (allActionsOnHit) {
        _hitBehavior = HitBehavior::AllActions;
    }
    else {
        _hitBehavior = HitBehavior::SingleAction;
    }
    if (ImGui::InputFloat("Bounce radius", &_bounceRadius)) {
        result = ImGuiResult::NeedsInit;
    }
    ImGui::Checkbox("Flow polarity", &_flowPolarity);
    ImGui::InputDouble("Flow cooldown (beat)", &_flowCooldownBeatTime);
    ImGui::InputFloat("Cooldown quantize denom", &_cooldownQuantizeDenom);
    ImGui::Checkbox("Show beats left", &_showBeatsLeft);
    ImGui::Checkbox("Reset cooldown on any hit", &_resetCooldownOnAnyHit);
    ImGui::Checkbox("Init hittable", &_initHittable);
    imgui_util::InputEditorId("Active Region", &_activeRegionEditorId);
    if (ImGui::Button("Set Region color")) {
        if (ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_activeRegionEditorId, ne::EntityType::Base)) {
            Vec4 color = _modelColor;
            color._w = 0.2f;
            e->_modelColor = color;
        }
    }
    if (SeqAction::ImGui("Hit actions", _hitActions)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("On-All-Hit actions", _allHitActions)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("Off-cooldown actions", _offCooldownActions)) {
        result = ImGuiResult::NeedsInit;
    }

    return result;
}

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

    for (auto const& pAction : _hitActions) {
        pAction->Init(g);
    }
    for (auto const& pAction : _allHitActions) {
        pAction->Init(g);
    }

    for (auto const& pAction : _offCooldownActions) {
        pAction->Init(g);
    }

    ForceStringsSameLength(_name, _keyText, _buttons);

    _flowCooldownStartBeatTime = -1.0;
    _timeOfLastHit = -1.0;
    _velocity = Vec3();

    _hittable = _initHittable;
    _numHits = 0;

    _activeRegionId = ne::EntityId();
    if (ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_activeRegionEditorId, ne::EntityType::Base)) {
        _activeRegionId = e->_id;
    }
}

namespace {
bool PlayerWithinRadius(FlowPlayerEntity const& player, TypingEnemyEntity const& enemy, GameManager& g) {
    if (!enemy._activeRegionId.IsValid()) {
        return true;
    }
    if (ne::Entity* region = g._neEntityManager->GetEntity(enemy._activeRegionId)) {
        bool inside = geometry::IsPointInBoundingBox(player._transform.Pos(), region->_transform);
        return inside;
    }
    return true;
}
}

#define CHUNKED_SPHERE 1

void TypingEnemyEntity::UpdateDerived(GameManager& g, float dt) {      
    if (!IsActive(g)) {
        return;
    }

    bool playerWithinRadius = true;

    if (!g._editMode) {
        Vec3 p  = _transform.GetPos();
    
        Vec3 dp = _velocity * dt;
        p += dp;
        _transform.SetTranslation(p);

        if (_destroyIfOffscreenLeft) {
            Vec3 p = _transform.GetPos();
            float constexpr kMinX = -10.f;
            if (p._x < kMinX) {
                g._neEntityManager->TagForDestroy(_id);
            }
        }

        // TODO: we're computing twice per frame whether enemies are within this radius. BORING
        FlowPlayerEntity* player = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));
        if (player) {
            
            playerWithinRadius = PlayerWithinRadius(*player, *this, g);
        }        
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    if (_flowCooldownStartBeatTime > 0.f) {
        double const cooldownFinishBeatTime = _flowCooldownStartBeatTime + _flowCooldownBeatTime;
        if (beatTime >= cooldownFinishBeatTime) {
            for (auto& pAction : _offCooldownActions) {
                pAction->Execute(g);
            }
            _flowCooldownStartBeatTime = -1.0;
            _numHits = 0;
            _timeOfLastHit = -1.0;
        }
    }

    bool showControllerInputs;
    if (g._editMode) {
        showControllerInputs = g._editor->_showControllerInputs;
    } else {
        showControllerInputs = g._inputManager->IsUsingController();
    }
    
    {
        int numFadedButtons = 0;
        if (_flowCooldownStartBeatTime > 0.f || !_hittable || !playerWithinRadius) {
            numFadedButtons = _keyText.length();
        } else {
            numFadedButtons = _numHits;
        }
        float constexpr kFadeAlpha = 0.2f;
        if (showControllerInputs) {
            Transform t = _transform;
            float const size = 0.25f;
            t.SetScale(Vec3(size, size, size));
            t.SetTranslation(_transform.Pos() + Vec3(-size, 0.f, size));
            for (int i = 0; i < _buttons.length(); ++i) {
                InputManager::ControllerButton b = CharToButton(_buttons[i]);
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
            std::string_view fadedText = std::string_view(_keyText).substr(0, numFadedButtons);
            Vec4 color(1.f, 1.f, 1.f, kFadeAlpha);
            if (!fadedText.empty()) {
                // AHHHH ALLOCATION
                g._scene->DrawTextWorld(std::string(fadedText), _transform.GetPos(), kTextSize, color);
            }
            color._w = 1.f;
            std::string_view activeText = std::string_view(_keyText).substr(numFadedButtons);
            if (!activeText.empty()) {
                bool appendToPrevious = numFadedButtons > 0;
                // AHHHH ALLOCATION
                g._scene->DrawTextWorld(std::string(activeText), _transform.GetPos(), kTextSize, color, appendToPrevious);
            }
        }
    }

    // Maybe update color from getting hit
    // turn to FadeColor on hit, then fade back to regular color
    /*if (_timeOfLastHit >= 0.0) {
        double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double constexpr kFadeTime = 0.25;
        double fadeFactor = (beatTime - _timeOfLastHit) / kFadeTime;        
        fadeFactor = math_util::Clamp(fadeFactor, 0.0, 1.0);
        Vec4 constexpr kFadeColor(0.f, 0.f, 0.f, 1.f);
        if (fadeFactor == 1.0) {
            _timeOfLastHit = -1.0;
        }
        _currentColor = kFadeColor + fadeFactor * (_modelColor - kFadeColor);
    }   */

    if (_bounceRadius > 0.f) {
        Transform t = _transform;
        t.ApplyScale(Vec3(0.7f, 0.7f, 0.7f));
        Quaternion q = t.Quat();
        q.SetFromAngleAxis(90.f * kPi / 180.f, Vec3(1.f, 0.f, 0.f));
        Quaternion rot;
        rot.SetFromAngleAxis(beatTime, Vec3(0.f, 0.f, 1.f));
        Quaternion final = rot * q;
        t.SetQuat(final);
        BoundMeshPNU const* mesh = g._scene->GetMesh("chunked_sphere");
        renderer::ModelInstance& model = g._scene->DrawMesh(mesh, t.Mat4Scale(), _currentColor);
        float constexpr kMaxExplode = 1.f;
        // explode more as hits are taken
        float const hitFactor = static_cast<float>(_numHits) / _keyText.length();
        double constexpr kBoomTime = 0.25;
        float boomPhase = math_util::Clamp((beatTime - _timeOfLastHit) / kBoomTime, 0.0, 1.0);
        boomPhase = math_util::SmoothUpAndDown(boomPhase);
        float constexpr kMaxBoomFactor = 0.75f;

        model._explodeDist = hitFactor * kMaxExplode + kMaxBoomFactor * boomPhase;

        // TODOOOOO
        //if (_flowCooldownStartBeatTime > 0.f || !_hittable) {
        //    Vec4 color = _currentColor;
        //    //color._w = 0.25f;
        //    model._color = color;

        //    double const cooldownTimeElapsed = math_util::Clamp(beatTime - _flowCooldownStartBeatTime, 0.0, _flowCooldownBeatTime);
        //    float factor = 1.f;
        //    if (_flowCooldownStartBeatTime > 0.f) {
        //        factor = 1.f - static_cast<float>(cooldownTimeElapsed / _flowCooldownBeatTime);
        //        factor = math_util::SmoothStep(factor);
        //    }            
        //    model._explodeDist = factor * kMaxExplode;
        //}
    }
    else if (_flowCooldownBeatTime > 0.f || !playerWithinRadius) {
        int constexpr kNumStepsX = 2;
        int constexpr kNumStepsZ = 2;
        float constexpr xStep = 1.f / (float) kNumStepsX;
        float constexpr zStep = 1.f / (float) kNumStepsZ;
        Mat4 localToWorld = _transform.Mat4Scale();
        Mat4 subdivMat = localToWorld;
        // subdivMat.Scale(xStep * 0.8f, 1.f, zStep * 0.8f);
        subdivMat.Scale(xStep * 0.8f, xStep * 0.8f, zStep * 0.8f);
        Vec4 color = _currentColor;
        // We adjust the scale of localToWorld AFTER setting subdivMat so that
        // changing the scale of localToWorld doesn't make the subdiv dudes
        // bigger
        if (_flowCooldownStartBeatTime > 0.f || !_hittable) {
            color._w = 0.5f;
            // timeLeft: [0, flowCooldown] --> [1, explodeScale]
            float constexpr kExplodeMaxScale = 2.f;
            double const cooldownTimeElapsed = math_util::Clamp(beatTime - _flowCooldownStartBeatTime, 0.0, _flowCooldownBeatTime);
            float factor = 1.f;
            if (_flowCooldownStartBeatTime > 0.f) {
                factor = 1.f - static_cast<float>(cooldownTimeElapsed / _flowCooldownBeatTime);
                factor = math_util::SmoothStep(factor);
            }
            float explodeScale = 1.f + factor * (kExplodeMaxScale - 1.f);
            localToWorld.Scale(explodeScale, 1.f, explodeScale);
            if (_showBeatsLeft) {
                float constexpr kBeatCellSize = 0.25f;
                float constexpr kSpacing = 0.1f;
                float constexpr kRowSpacing = 0.1f;
                float constexpr kRowLength = 4.f * kBeatCellSize + (3.f) * kSpacing;
                float const totalBeats = std::ceil(_flowCooldownBeatTime);
                int const numRows = ((static_cast<int>(totalBeats) - 1) / 4) + 1;
                float const vertSize = numRows * kBeatCellSize + (numRows - 1) * kRowSpacing;
                float const rowStartXPos = _transform.GetPos()._x - 0.5f * kRowLength;
                float const rowStartZPos = _transform.GetPos()._z - 0.5f - vertSize;
                Vec3 firstBeatPos = _transform.GetPos() + Vec3(rowStartXPos, 0.f, rowStartZPos);
                Mat4 m;
                m.SetTranslation(firstBeatPos);
                m.ScaleUniform(kBeatCellSize);
                int const numLeft = static_cast<int>(std::ceil(_flowCooldownBeatTime - cooldownTimeElapsed));
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
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    ++_numHits;
    if (_numHits == _keyText.length()) {
        for (auto const& pAction : _allHitActions) {
            pAction->Execute(g);
        }
    }
    if (_numHits >= _keyText.length()) {
        _numHits = _keyText.length();
        if (_cooldownQuantizeDenom > 0.f) {
            _flowCooldownStartBeatTime = BeatClock::GetNextBeatDenomTime(beatTime, _cooldownQuantizeDenom);
        } else {
            _flowCooldownStartBeatTime = beatTime;
        }
        if (_destroyAfterTyped) {
            bool success = g._neEntityManager->TagForDeactivate(_id);
            assert(success);
        }
    }
    _timeOfLastHit = beatTime;

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
        _flowCooldownStartBeatTime = -1.0;
        _numHits = 0;
        _timeOfLastHit = -1.0;
    }
}

InputManager::Key TypingEnemyEntity::GetNextKey() const {
    int textIndex = std::min(_numHits, (int) _keyText.length() - 1);
    char nextChar = std::tolower(_keyText.at(textIndex));
    int charIx = nextChar - 'a';
    if (charIx < 0 || charIx > static_cast<int>(InputManager::Key::Z)) {
        printf("WARNING: char \'%c\' not in InputManager!\n", nextChar);
        return InputManager::Key::NumKeys;
    }
    InputManager::Key nextKey = static_cast<InputManager::Key>(charIx);
    return nextKey;
}

InputManager::ControllerButton TypingEnemyEntity::GetNextButton() const {
    int textIndex = std::min(_numHits, (int)_buttons.length() - 1);
    char nextChar = std::tolower(_buttons.at(textIndex));
    return CharToButton(nextChar);
}

void TypingEnemyEntity::OnEditPick(GameManager& g) {
    DoHitActions(g);
}

bool TypingEnemyEntity::CanHit(FlowPlayerEntity const& player, GameManager& g) const {
    if (!PlayerWithinRadius(player, *this, g)) {
        return false;
    }
    if (!_hittable) {
        return false;
    }
    return _flowCooldownStartBeatTime <= 0.0;
}

static TypingEnemyEntity sMultiEnemy;

void TypingEnemyEntity::MultiSelectImGui(GameManager& g, std::vector<TypingEnemyEntity*>& enemies) {

    ImGui::InputInt("Entity tag", &sMultiEnemy._tag);
    imgui_util::ColorEdit4("Model color", &sMultiEnemy._modelColor);
    ImGui::InputInt("Section ID", &sMultiEnemy._flowSectionId);
    
    // if (ImGui::Button("Add Action")) {
    //     sMultiEnemy._hitActionStrings.emplace_back();
    // }
    // int deletedIx = -1;
    // for (int i = 0; i < sMultiEnemy._hitActionStrings.size(); ++i) {
    //     ImGui::PushID(i);
    //     std::string& actionStr = sMultiEnemy._hitActionStrings[i];
    //     imgui_util::InputText<256>("Action", &actionStr);
    //     if (ImGui::Button("Delete")) {
    //         deletedIx = i;
    //     }
    //     ImGui::PopID();
    // }

    // if (deletedIx >= 0) {
    //     sMultiEnemy._hitActionStrings.erase(sMultiEnemy._hitActionStrings.begin() + deletedIx);
    // }

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
            e->_keyText = std::string(1, letter);
        }
    }
}

void TypingEnemyEntity::SetHittable(bool hittable) {
    _hittable = hittable;
}
