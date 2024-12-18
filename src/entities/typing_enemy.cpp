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
#include "particle_mgr.h"
#include "entities/particle_emitter.h"
#include "imgui_vector_util.h"
#include "typing_enemy_mgr.h"
#include "midi_util.h"

#include "actions.h"

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

void HitResponse::Load(serial::Ptree pt) {
    serial::TryGetEnum(pt, "type", _type);
    pt.TryGetFloat("speed", &_speed);
    pt.TryGetFloat("push_angle_deg", &_pushAngleDeg);
    pt.TryGetFloat("time", &_time);
    pt.TryGetFloat("ps_time", &_pullStopTime);
    pt.TryGetBool("stop_on_pass", &_stopOnPass);
    pt.TryGetBool("stop_after_timer", &_stopAfterTimer);
}
void HitResponse::Save(serial::Ptree pt) const {
    serial::PutEnum(pt, "type", _type);
    pt.PutFloat("speed", _speed);
    pt.PutFloat("push_angle_deg", _pushAngleDeg);
    pt.PutFloat("time", _time);
    pt.PutFloat("ps_time", _pullStopTime);
    pt.PutBool("stop_on_pass", _stopOnPass);
    pt.PutBool("stop_after_timer", _stopAfterTimer);
}
bool HitResponse::ImGui(char const* label) {
    bool changed = false;
    if (ImGui::TreeNode(label)) {
        changed = HitResponseTypeImGui("Type", &_type) || changed;
        changed = ImGui::InputFloat("Speed", &_speed) || changed;
        changed = ImGui::InputFloat("PushAngleDeg", &_pushAngleDeg) || changed;
        changed = ImGui::InputFloat("Time", &_time) || changed;
        changed = ImGui::InputFloat("Pullstop time", &_pullStopTime) || changed;
        changed = ImGui::Checkbox("StopOnPass", &_stopOnPass) || changed;
        changed = ImGui::Checkbox("StopAfterTimer", &_stopAfterTimer) || changed;
        ImGui::TreePop();
    }
    return changed;
}

void TypingEnemyEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();

    int constexpr kRefactor231024Version = 1;
    int constexpr kHitResponse231115Version = 2;
    int const scriptVersion = pt.GetVersion();
    if (scriptVersion < kRefactor231024Version) {
        bool flowPolarity = false;
        pt.TryGetBool("flow_polarity", &flowPolarity);
        if (flowPolarity) {
            _p._hitResponse._type = HitResponseType::Push;
        } else {
            _p._hitResponse._type = HitResponseType::Pull;
        }

    } else if (scriptVersion < kHitResponse231115Version) {
        pt.TryGetFloat("dash_vel", &_p._hitResponse._speed);
        pt.TryGetFloat("push_angle_deg", &_p._hitResponse._pushAngleDeg);
        pt.TryGetBool("stop_on_pass", &_p._hitResponse._stopOnPass);
        _p._lastHitResponse = _p._hitResponse;
        if (!serial::TryGetEnum(pt, "hit_response_type", _p._hitResponse._type)) {
            // backward compat
            serial::TryGetEnum(pt, "enemy_type", _p._hitResponse._type);            
        } else {
            pt.TryGetBool("use_last_hit_response", &_p._useLastHitResponse);
            serial::TryGetEnum(pt, "last_hit_response_type", _p._lastHitResponse._type);
            pt.TryGetFloat("last_hit_vel", &_p._lastHitResponse._speed);
            pt.TryGetFloat("last_hit_push_angle_deg", &_p._lastHitResponse._pushAngleDeg);
        }
    } else {
        serial::LoadFromChildOf(pt, "hit_response", _p._hitResponse);
        pt.TryGetBool("use_last_hit_response", &_p._useLastHitResponse);
        serial::LoadFromChildOf(pt, "last_hit_response", _p._lastHitResponse);
    }
    
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
    pt.TryGetDouble("timed_hittable_time", &_p._timedHittableTime);
    serial::TryGetEnum(pt, "region_type", _p._regionType);
    serial::LoadFromChildOf(pt, "active_region_editor_id", _p._activeRegionEditorId);
    pt.TryGetBool("lock_player_in_region", &_p._lockPlayerOnEnterRegion);
    pt.TryGetBool("allow_backward", &_p._allowTypeBackward);
    pt.TryGetBool("just_draw_model", &_p._justDrawModel);
    pt.TryGetInt("group_id", &_p._groupId);

    SeqAction::LoadActionsFromChildNode(pt, "hit_actions", _p._hitActions);
    SeqAction::LoadActionsFromChildNode(pt, "all_hit_actions", _p._allHitActions);
    SeqAction::LoadActionsFromChildNode(pt, "off_cooldown_actions", _p._offCooldownActions);
    SeqAction::LoadActionsFromChildNode(pt, "combo_end_actions", _p._comboEndActions);

    if (gRandomLetters) {
        if (gCurrentShuffleIx > 25) {
            // reshuffle
            for (int i = 0; i < 26; ++i) {
                int swapIx = rng::GetIntGlobal(i, 25);
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
    serial::SaveInNewChildOf(pt, "hit_response", _p._hitResponse);
    pt.PutBool("use_last_hit_response", _p._useLastHitResponse);
    serial::SaveInNewChildOf(pt, "last_hit_response", _p._lastHitResponse);   
    
    pt.PutString("text", _p._keyText.c_str());
    pt.PutString("buttons", _p._buttons.c_str());
    pt.PutBool("type_kill", _p._destroyAfterTyped);
    pt.PutBool("all_actions_on_hit", _p._hitBehavior == HitBehavior::AllActions);
    pt.PutDouble("flow_cooldown", _p._flowCooldownBeatTime);
    pt.PutBool("show_beats_left", _p._showBeatsLeft);
    pt.PutFloat("cooldown_quantize_denom", _p._cooldownQuantizeDenom);
    pt.PutBool("reset_cooldown_on_any_hit", _p._resetCooldownOnAnyHit);
    pt.PutBool("init_hittable", _p._initHittable);
    pt.PutDouble("timed_hittable_time", _p._timedHittableTime);
    pt.PutBool("allow_backward", _p._allowTypeBackward);
    pt.PutBool("just_draw_model", _p._justDrawModel);
    pt.PutInt("group_id", _p._groupId);
    
    serial::PutEnum(pt, "region_type", _p._regionType);
    serial::SaveInNewChildOf(pt, "active_region_editor_id", _p._activeRegionEditorId);
    pt.PutBool("lock_player_in_region", _p._lockPlayerOnEnterRegion);
    SeqAction::SaveActionsInChildNode(pt, "hit_actions", _p._hitActions);
    SeqAction::SaveActionsInChildNode(pt, "all_hit_actions", _p._allHitActions);
    SeqAction::SaveActionsInChildNode(pt, "off_cooldown_actions", _p._offCooldownActions);
    SeqAction::SaveActionsInChildNode(pt, "combo_end_actions", _p._comboEndActions);
}

ne::BaseEntity::ImGuiResult TypingEnemyEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;

    _p._hitResponse.ImGui("HitResponse");
    ImGui::Checkbox("UseLastHitResponse", &_p._useLastHitResponse);
    if (_p._useLastHitResponse) {
        _p._lastHitResponse.ImGui("LastHitResponse");
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
    if (ImGui::InputInt("Group ID", &_p._groupId)) {
        result = ImGuiResult::NeedsInit;
    }
    ImGui::Checkbox("Just draw model", &_p._justDrawModel);
    ImGui::InputDouble("Flow cooldown (beat)", &_p._flowCooldownBeatTime);
    ImGui::InputFloat("Cooldown quantize denom", &_p._cooldownQuantizeDenom);
    ImGui::Checkbox("Show beats left", &_p._showBeatsLeft);
    ImGui::Checkbox("Reset cooldown on any hit", &_p._resetCooldownOnAnyHit);
    ImGui::Checkbox("Init hittable", &_p._initHittable);
    ImGui::InputDouble("Timed hittable (beat)", &_p._timedHittableTime);
    ImGui::Checkbox("Allow backward", &_p._allowTypeBackward);
    EnemyRegionTypeImGui("Region type", &_p._regionType);
    switch (_p._regionType) {
        case EnemyRegionType::None: break;
        case EnemyRegionType::TriggerEntity: {
            imgui_util::InputEditorId("Active Region", &_p._activeRegionEditorId);
            break;
        }
        case EnemyRegionType::BoundedXAxis: {
            break;
        }
        case EnemyRegionType::Count: assert(false); break;
    }
    ImGui::Checkbox("Lock player in region", &_p._lockPlayerOnEnterRegion);
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

namespace {
void AddToMidiNotes(GameManager &g, ne::BaseEntity **entities, size_t entityCount, int octaveChange) {
    for (int entityIx = 0; entityIx < entityCount; ++entityIx) {
        TypingEnemyEntity *e = (TypingEnemyEntity *)entities[entityIx];
        for (int actionIx = 0; actionIx < e->_p._hitActions.size(); ++actionIx) {
            std::unique_ptr<SeqAction> &pAction = e->_p._hitActions[actionIx];
            if (!pAction) {
                printf("typing_enemy.cpp:AddToMidiNotes: null pAction, wtf?\n");
                continue;
            }
            SeqAction &baseAction = *pAction;
            if (baseAction.Type() == SeqActionType::NoteOnOff) {
                NoteOnOffSeqAction &action = static_cast<NoteOnOffSeqAction&>(baseAction);
                std::vector<MidiNoteAndName> &midiNotes = action._props._midiNotes;
                for (int noteIx = 0; noteIx < midiNotes.size(); ++noteIx) {
                    MidiNoteAndName &n = midiNotes[noteIx];
                    if (n._note < 0) {
                        continue;
                    }
                    n._note += octaveChange;
                    n._note = math_util::Clamp(n._note, kMidiMinNote, kMidiMaxNote);
                }
            }
        }
    }
}
}

ne::BaseEntity::ImGuiResult TypingEnemyEntity::MultiImGui(GameManager& g, BaseEntity** entities, size_t entityCount) {
    bool needsInit = false;

    if (ImGui::Checkbox("Kill on type", &_p._destroyAfterTyped)) {
        imgui_util::SetMemberOfEntities(&Props::_destroyAfterTyped, *this, entities, entityCount);
    }

    if (ImGui::Checkbox("Reset cooldown on any hit", &_p._resetCooldownOnAnyHit)) {
        imgui_util::SetMemberOfEntities(&Props::_resetCooldownOnAnyHit, *this, entities, entityCount);
    }

    if (ImGui::InputDouble("Flow cooldown (beat)", &_p._flowCooldownBeatTime)) {
        imgui_util::SetMemberOfEntities(&Props::_flowCooldownBeatTime, *this, entities, entityCount);
    }

    if (ImGui::InputInt("Group ID", &_p._groupId)) {
        imgui_util::SetMemberOfEntities(&Props::_groupId, *this, entities, entityCount);
    }

    if (ImGui::Button("Increase octave")) {
        AddToMidiNotes(g, entities, entityCount, 12);
    }
    ImGui::SameLine();
    if (ImGui::Button("Decrease octave")) {
        AddToMidiNotes(g, entities, entityCount, -12);
    }

    return needsInit ? ne::BaseEntity::ImGuiResult::NeedsInit : ne::BaseEntity::ImGuiResult::Done;
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

    if (_p._groupId >= 0) {
        TypingEnemyMgr_AddEnemy(*g._typingEnemyMgr, g, *this);
    }
}

namespace {
bool PlayerWithinRadius(FlowPlayerEntity const& player, TypingEnemyEntity const& enemy, GameManager& g) {
    switch (enemy._p._regionType) {
        case EnemyRegionType::None: {
            return true;
        }
        case EnemyRegionType::TriggerEntity: {
            if (ne::Entity* region = g._neEntityManager->GetEntity(enemy._s._activeRegionId)) {
                //bool inside = geometry::IsPointInBoundingBox(player._transform.Pos(), region->_transform);
                bool inside = geometry::DoAABBsOverlap(player._transform, region->_transform, nullptr);
                return inside;
            }
            return true;
        }
        case EnemyRegionType::BoundedXAxis: {
            Vec3 enemyToPlayer = player._transform.Pos() - enemy._transform.Pos();
            Vec3 xAxis = enemy._transform.GetXAxis();
            float xcomp = Vec3::Dot(enemyToPlayer, xAxis);
            Vec3 xProj = xcomp * xAxis;
            Vec3 orthog = enemyToPlayer - xProj;
            float orthogLen2 = orthog.Length2();
            float halfZScale = 0.5f * enemy._transform.Scale()._z;
            bool inside = orthogLen2 < halfZScale*halfZScale;
            return inside;
        }
        case EnemyRegionType::Count: {
            assert(false);
            return true;
        }
    }
    assert(false);
    return true;
}

void DrawTicks(int activeCount, int totalCount, float const dt, Transform const& renderTrans, float scale, GameManager& g, TypingEnemyEntity& e) {
    float constexpr kBeatCellSize = 0.25f;
    float constexpr kSpacing = 0.1f;
    float constexpr kRowSpacing = 0.1f;
    float constexpr kRowLength = 4.f * kBeatCellSize + (3.f) * kSpacing;
    int const numRows = ((totalCount - 1) / 4) + 1;
    float const vertSize = numRows * kBeatCellSize + (numRows - 1) * kRowSpacing;
    float const rowStartXPos = renderTrans.GetPos()._x - 0.5f * kRowLength;
    float const rowStartZPos = renderTrans.GetPos()._z - 0.5f - vertSize;
    Vec3 firstBeatPos = renderTrans.GetPos() + Vec3(rowStartXPos, 0.f, rowStartZPos);
    Mat4 m;
    m.SetTranslation(firstBeatPos);
    m.ScaleUniform(kBeatCellSize * scale);
    Vec4 color(0.f, 1.f, 0.f, 1.f);
    for (int row = 0, beatIx = 0; row < numRows; ++row) {
        for (int col = 0; col < 4 && beatIx < totalCount; ++col, ++beatIx) {
            if (beatIx >= activeCount) {
                color.Set(0.3f, 0.3f, 0.3f, 1.f);
            }
            Vec3 p(rowStartXPos + col * (kSpacing + kBeatCellSize), 0.f, rowStartZPos + row * (kRowSpacing + kBeatCellSize));
            m.SetTranslation(p);
            g._scene->DrawCube(m, color);
        }
    }
}

namespace {
    void DrawText(GameManager &g, TypingEnemyEntity &e, Transform const &renderTrans, bool hittable, bool showControllerInputs) {
        int numFadedButtons = 0;
        if (hittable) {
            numFadedButtons = e._s._numHits;            
        } else {
            numFadedButtons = e._p._keyText.length();
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
            Transform textTrans = renderTrans;
            textTrans.Translate(Vec3(0.25f, 0.f, 0.25f));
            float constexpr kTextSize = 1.5f;
            std::string_view fadedText = std::string_view(e._p._keyText).substr(0, numFadedButtons);
            Vec4 color(1.f, 1.f, 1.f, kFadeAlpha);
            if (!fadedText.empty()) {
                g._scene->DrawTextWorld(fadedText, textTrans.GetPos(), kTextSize, color);
            }
            color._w = 1.f;
            std::string_view activeText = std::string_view(e._p._keyText).substr(numFadedButtons);
            if (!activeText.empty()) {
                bool appendToPrevious = numFadedButtons > 0;
                g._scene->DrawTextWorld(activeText, textTrans.GetPos(), kTextSize, color, appendToPrevious);
            }
        }
    }
}

void DrawPullEnemy(float const dt, GameManager& g, TypingEnemyEntity& e, Transform const &renderTrans, bool hittable, bool showControllerInputs) {
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    DrawText(g, e, renderTrans, hittable, showControllerInputs);
    
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
    if (!hittable) {
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
            int const numLeft = static_cast<int>(std::ceil(e._p._flowCooldownBeatTime - cooldownTimeElapsed));
            DrawTicks(numLeft, (int) std::ceil(e._p._flowCooldownBeatTime), dt, renderTrans, 1.f, g, e);
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

    if (e._p._timedHittableTime > 0.0) {
        double beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double beatMod = std::fmod(beatTime, e._p._timedHittableTime);
        int beatProgressCount = (int)std::ceil(beatMod);
        float scale = 1.f;
        if (beatMod >= (e._p._timedHittableTime - 1.f)) {
            // 0 at last beat, 1 at downbeat
            float beatLeft = beatMod - (e._p._timedHittableTime - 1.f);
            scale = 1.f - math_util::SmoothStep(beatLeft);
        }
        if (scale > 0.1f) {
            DrawTicks(beatProgressCount, (int)e._p._timedHittableTime, dt, renderTrans, scale, g, e);
        }
    }
}

void DrawPushEnemy(float const dt, GameManager& g, TypingEnemyEntity& e, Transform const &renderTrans, bool hittable, bool showControllerInputs) {
    DrawPullEnemy(dt, g, e, renderTrans, hittable, showControllerInputs);
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

    
    if (_p._timedHittableTime > 0.0) {
        double beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double beatMod = std::fmod(beatTime, _p._timedHittableTime);
        bool hittable = beatMod < 0.25 || (beatMod > _p._timedHittableTime - 0.5);
        _s._hittable = hittable;
    }

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
    bool showControllerInputs;
    if (g._editMode) {
        showControllerInputs = g._editor->_showControllerInputs;
    } else {
        showControllerInputs = g._inputManager->IsUsingController();
    }

    Transform renderTrans = _transform;
    // Apply bump logic
    if (_s._bumpTimer >= 0.f) {
        float constexpr kTotalBumpTime = 0.1f;
        float constexpr kBumpAmount = 0.15f;
        float factor = math_util::Clamp(_s._bumpTimer / kTotalBumpTime, 0.f, 1.f);
        factor = math_util::Triangle(factor);
        float dist = factor * kBumpAmount;
        Vec3 p = renderTrans.Pos();
        p += _s._bumpDir * dist;
        renderTrans.SetPos(p);
        _s._bumpTimer += dt;
        if (_s._bumpTimer > kTotalBumpTime) {
            _s._bumpTimer = -1.f;
        }
    }

    FlowPlayerEntity* player = static_cast<FlowPlayerEntity*>(g._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer));

    bool playerWithinRadius = true;
    // TODO: we're computing twice per frame whether enemies are within this radius. BORING    
    if (player) {
        playerWithinRadius = PlayerWithinRadius(*player, *this, g);
    }

    bool hittable = _s._flowCooldownStartBeatTime <= 0.f && _s._hittable && playerWithinRadius;

    if (_p._justDrawModel) {
        BaseEntity::Draw(g, dt);
        DrawText(g, *this, renderTrans, hittable, showControllerInputs);
        return;
    }    

    // TODO: pass the above data into DrawPullEnemy too
    switch (_p._hitResponse._type) {
        case HitResponseType::None:
            DrawPullEnemy(dt, g, *this, renderTrans, hittable, showControllerInputs);
            break;
        case HitResponseType::Pull:
            DrawPullEnemy(dt, g, *this, renderTrans, hittable, showControllerInputs);
            break;
        case HitResponseType::Push:
            DrawPushEnemy(dt, g, *this, renderTrans, hittable, showControllerInputs);
            break;
        case HitResponseType::Count:
            assert(false);
            break;
    }    
}

HitResult TypingEnemyEntity::OnHit(GameManager& g, int hitKeyIndex) {
    HitResult result;

    int hitActionIx = -1;
    if (hitKeyIndex == 0) {
        // FORWARD
        hitActionIx = _s._numHits;
        ++_s._numHits;
        if (_s._numHits == _p._keyText.length()) {
            for (auto const& pAction : _p._allHitActions) {
                pAction->Execute(g);
                std::unique_ptr<SeqAction> clone = SeqAction::Clone(*pAction);
                result._heldActions.push_back(std::move(clone));
            }
        }
    } else if (hitKeyIndex == 1) {
        // BACKWARD
        --_s._numHits;
        hitActionIx = _s._numHits;
        _s._numHits = std::max(0, _s._numHits); 
    } else {
        printf("TypingEnemyEntity::OnHit: Unhandled hitKeyIndex %d\n", hitKeyIndex);
        return result;
    }
    hitActionIx = math_util::Clamp(hitActionIx, 0, _p._hitActions.size() - 1);
     
    DoHitActions(g, hitActionIx, result._heldActions);
    if (g._editMode && _s._numHits >= _p._keyText.length()) {
        _s._numHits = 0;
    }

    if (g._editMode) {
        return result;
    }

    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    _s._timeOfLastHit = beatTime;

    if (_p._useLastHitResponse && _s._numHits >= _p._keyText.length()) {
        result._response = _p._lastHitResponse;
    } else {
        result._response = _p._hitResponse;
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

#if 0
            ParticleEmitterEntity* e = static_cast<ParticleEmitterEntity*>(g._neEntityManager->AddEntity(ne::EntityType::ParticleEmitter));
            e->_initTransform = _transform;
            if (result._response._type == HitResponseType::Push) {
                float angleRad = result._response._pushAngleDeg * kDeg2Rad;
                Vec3 z(cos(angleRad), 0.f, -sin(angleRad));
                z = -z;
                Vec3 y(0.f, 1.f, 0.f);
                Vec3 x = Vec3::Cross(y, z);
                Mat3 rot;
                rot.SetCol(0, x);
                rot.SetCol(1, y);
                rot.SetCol(2, z);
                Quaternion q;
                q.SetFromRotMat(rot);
                e->_initTransform.SetQuat(q);
            }
            e->_modelColor = _modelColor;
            e->Init(g);
#else
            Vec3 boxP = _transform.Pos();
            // TODO: Assumes AABB
            Vec3 minP = boxP - _transform.Scale() * 0.5f;
            Vec3 maxP = boxP + _transform.Scale() * 0.5f;
            for (int ii = 0; ii < 20; ++ii) {
                Vec3 p;
                p._x = rng::GetFloatGlobal(minP._x, maxP._x);
                p._y = rng::GetFloatGlobal(minP._y, maxP._y);
                p._z = rng::GetFloatGlobal(minP._z, maxP._z);
                Vec3 dir = p - boxP;
                dir.Normalize();

                Particle* particle = g._particleMgr->SpawnParticle();
                particle->t.SetPos(p);
                particle->t.SetScale(Vec3(0.15f, 0.15f, 0.15f));
                particle->v = 2.f * dir;
                particle->rotV = 4.f * dir;
                particle->color = _modelColor;
                particle->alphaV = -2.f;
                particle->timeLeft = 1.f;
                particle->shape = ParticleShape::Cube;
            }
#endif
        }
    }
      
    return result;
}

void TypingEnemyEntity::DoHitActions(GameManager& g, int actionIx, std::vector<std::unique_ptr<SeqAction>>& clones) {
    if (_p._hitActions.empty()) {
        return;
    }
    switch (_p._hitBehavior) {
        case HitBehavior::SingleAction: {
            SeqAction& a = *_p._hitActions[actionIx];

            a.Execute(g);
            std::unique_ptr<SeqAction> clone = SeqAction::Clone(a);
            clones.push_back(std::move(clone));
            break;
        }
        case HitBehavior::AllActions: {
            for (auto const& pAction : _p._hitActions) {
                pAction->Execute(g);
                std::unique_ptr<SeqAction> clone = SeqAction::Clone(*pAction);
                clones.push_back(std::move(clone));
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

void TypingEnemyEntity::ResetCooldown() {
    _s._flowCooldownStartBeatTime = -1.0;
    _s._numHits = 0;
    _s._timeOfLastHit = -1.0;
}

void TypingEnemyEntity::OnHitOther(GameManager& g) {
    if (_p._resetCooldownOnAnyHit) {
        //_s._flowCooldownStartBeatTime = -1.0;
        ResetCooldown();
    }
    /*_s._numHits = 0;
    _s._timeOfLastHit = -1.0;*/
}

TypingEnemyEntity::NextKeys TypingEnemyEntity::GetNextKeys() const {
    TypingEnemyEntity::NextKeys nextKeys;
    nextKeys.fill(InputManager::Key::NumKeys);
    if (_p._keyText.empty()) {
        return nextKeys;
    }
    int keyIx = 0;
    int textIndex = std::min(_s._numHits, (int) _p._keyText.length() - 1);
    char nextChar = _p._keyText.at(textIndex);
    nextKeys[keyIx++] = InputManager::CharToKey(nextChar);
    if (_p._allowTypeBackward) {
        if (textIndex > 0) {
            char prevChar = _p._keyText.at(textIndex - 1);
            nextKeys[keyIx++] = InputManager::CharToKey(prevChar);
        }
    }
    return nextKeys;
}

InputManager::ControllerButton TypingEnemyEntity::GetNextButton() const {
    if (_p._buttons.empty()) {
        return InputManager::ControllerButton::Count;
    }
    int textIndex = std::min(_s._numHits, (int)_p._buttons.length() - 1);
    char nextChar = std::tolower(_p._buttons.at(textIndex));
    return CharToButton(nextChar);
}

void TypingEnemyEntity::OnEditPick(GameManager& g) {
    OnHit(g, 0);
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
            int letterIx = rng::GetIntGlobal(0, 25);
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

void TypingEnemyEntity::RandomizeText() {
    for (int ii = 0; ii < _p._keyText.length(); ++ii) {
        _p._keyText[ii] = (char)(rng::GetIntGlobal((int)'a', (int)'z'));
    }
}
