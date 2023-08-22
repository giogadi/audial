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

extern GameManager gGameManager;

extern bool gRandomLetters;

namespace {
    int gShuffledLetterIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
    int gCurrentShuffleIx = 26; // one past
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

    _activeRadius = -1.f;
    pt.TryGetFloat("active_radius", &_activeRadius);

    SeqAction::LoadActionsFromChildNode(pt, "hit_actions", _hitActions);
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
        _text.resize(1);
        _text[0] = 'a' + randLetterIx;
    }
}

void TypingEnemyEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutString("text", _text.c_str());
    pt.PutBool("type_kill", _destroyAfterTyped);
    pt.PutBool("all_actions_on_hit", _hitBehavior == HitBehavior::AllActions);
    pt.PutBool("flow_polarity", _flowPolarity);
    pt.PutDouble("flow_cooldown", _flowCooldownBeatTime);
    pt.PutBool("show_beats_left", _showBeatsLeft);
    pt.PutFloat("cooldown_quantize_denom", _cooldownQuantizeDenom);
    pt.PutBool("reset_cooldown_on_any_hit", _resetCooldownOnAnyHit);
    pt.PutBool("init_hittable", _initHittable);
    pt.PutFloat("active_radius", _activeRadius);
    SeqAction::SaveActionsInChildNode(pt, "hit_actions", _hitActions);
    SeqAction::SaveActionsInChildNode(pt, "off_cooldown_actions", _offCooldownActions);
}

ne::BaseEntity::ImGuiResult TypingEnemyEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;

    imgui_util::InputText<32>("Text", &_text);
    ImGui::Checkbox("Kill on type", &_destroyAfterTyped);

    bool allActionsOnHit = _hitBehavior == HitBehavior::AllActions;
    ImGui::Checkbox("All actions on hit", &allActionsOnHit);
    if (allActionsOnHit) {
        _hitBehavior = HitBehavior::AllActions;
    }
    else {
        _hitBehavior = HitBehavior::SingleAction;
    }

    ImGui::Checkbox("Flow polarity", &_flowPolarity);
    ImGui::InputDouble("Flow cooldown (beat)", &_flowCooldownBeatTime);
    ImGui::InputFloat("Cooldown quantize denom", &_cooldownQuantizeDenom);
    ImGui::Checkbox("Show beats left", &_showBeatsLeft);
    ImGui::Checkbox("Reset cooldown on any hit", &_resetCooldownOnAnyHit);
    ImGui::Checkbox("Init hittable", &_initHittable);
    ImGui::InputFloat("Active radius", &_activeRadius);
    if (SeqAction::ImGui("Hit actions", _hitActions)) {
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

    for (auto const& pAction : _offCooldownActions) {
        pAction->Init(g);
    }

    _flowCooldownStartBeatTime = -1.0;
    _timeOfLastHit = -1.0;
    _velocity = Vec3();

    _hittable = _initHittable;
    _numHits = 0;
}

namespace {
    bool PlayerWithinRadius(TypingEnemyEntity const& enemy) {
        if (enemy._activeRadius < 0.f) {
            return true;
        }
        ne::Entity* e = gGameManager._neEntityManager->GetFirstEntityOfType(ne::EntityType::FlowPlayer);
        FlowPlayerEntity* player = static_cast<FlowPlayerEntity*>(e);
        Vec3 dp = player->_transform.GetPos() - enemy._transform.GetPos();
        return std::abs(dp._x) < enemy._activeRadius && std::abs(dp._z) < enemy._activeRadius;
    }
}

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

        playerWithinRadius = PlayerWithinRadius(*this);
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    if (_flowCooldownStartBeatTime > 0.f) {
        double const cooldownFinishBeatTime = _flowCooldownStartBeatTime + _flowCooldownBeatTime;
        if (beatTime >= cooldownFinishBeatTime) {
            for (auto& pAction : _offCooldownActions) {
                pAction->Execute(g);
            }
            _flowCooldownStartBeatTime = -1.0;
        }
    }

    float constexpr kTextSize = 1.5f;
    if (_flowCooldownStartBeatTime > 0.f || !playerWithinRadius || !_hittable) {
        Vec4 color = _textColor;
        color._w = 0.2f;
        g._scene->DrawTextWorld(_text, _transform.GetPos(), kTextSize, color);
    } else if (_text.length() > 1) {
        if (_numHits > 0) {
            std::string_view substr = std::string_view(_text).substr(0, _numHits);
            g._scene->DrawTextWorld(std::string(substr), _transform.GetPos(), kTextSize, Vec4(1.f, 1.f, 0.f, 1.f));
        }
        if (_numHits < _text.length()) {
            std::string_view substr = std::string_view(_text).substr(_numHits);
            g._scene->DrawTextWorld(std::string(substr), _transform.GetPos(), kTextSize, _textColor);
        }
    } else {
        g._scene->DrawTextWorld(_text, _transform.GetPos(), kTextSize, _textColor);
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

    if (_activeRadius >= 0.f) {
        Transform t = _transform;
        Vec3 scale = 2.f * _activeRadius * Vec3(1.f, 1.f, 1.f);
        scale._y = 0.1f;
        t.SetScale(2.f * _activeRadius * Vec3(1.f, 0.1f, 1.f));        
        t.SetPosY(_transform.GetPos()._y - 1.f);
        t.SetQuat(Quaternion());        
        Vec4 constexpr kRadiusColor(0.f, 1.f, 1.f, 0.25f);
        g._scene->DrawCube(t.Mat4Scale(), kRadiusColor);
    }

    if (_flowCooldownBeatTime > 0.f || !playerWithinRadius) {
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
    if (_cooldownQuantizeDenom > 0.f) {
        _flowCooldownStartBeatTime = BeatClock::GetNextBeatDenomTime(beatTime, _cooldownQuantizeDenom);
    }
    else {
        _flowCooldownStartBeatTime = beatTime;
    }
    ++_numHits;
    if (_numHits == _text.length()) {
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

void TypingEnemyEntity::OnEditPick(GameManager& g) {
    DoHitActions(g);
}

bool TypingEnemyEntity::CanHit() const {
    if (!PlayerWithinRadius(*this)) {
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
            e->_text = std::string(1, letter);
        }
    }
}

void TypingEnemyEntity::SetHittable(bool hittable) {
    _hittable = hittable;
}
