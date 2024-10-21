#include "flow_player.h"

#include "game_manager.h"
#include "typing_enemy.h"
#include "input_manager.h"
#include "renderer.h"
#include "camera.h"
#include "math_util.h"
#include "beat_clock.h"
#include "geometry.h"
#include "seq_action.h"
#include "imgui_util.h"

#include "entities/flow_pickup.h"
#include "entities/flow_wall.h"
#include "entities/flow_trigger.h"
#include "entities/vfx.h"

void FlowPlayerEntity::StartRespawn(GameManager& g) {
    _s._respawnStartBeatTime = g._beatClock->GetBeatTimeFromEpoch(); 
    _s._respawnHasResetScene = false;
    if (FlowTriggerEntity* e = g._neEntityManager->GetEntityAs<FlowTriggerEntity>(_s._deathStartTrigger)) {
        e->OnTrigger(g);
    }
    for (int ii = 0; ii < static_cast<size_t>(InputManager::Key::NumKeys); ++ii) {
        _s._haveSeenKeyUpSinceLastHit[ii] = true;
        _s._keyBufferTimeLeft[ii] = 0;
    }
}

void FlowPlayerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutFloat("launch_vel", _p._defaultLaunchVel);
    pt.PutFloat("max_horiz_speed_after_dash", _p._maxHorizSpeedAfterDash);
    pt.PutFloat("max_vert_speed_after_dash", _p._maxVertSpeedAfterDash);
    pt.PutFloat("max_overall_speed_after_dash", _p._maxOverallSpeedAfterDash);
    pt.PutFloat("dash_time", _p._dashTime);
    serial::SaveInNewChildOf(pt, "gravity", _p._gravity);
    pt.PutFloat("max_fall_speed", _p._maxFallSpeed);
    pt.PutBool("pull_manual_hold", _p._pullManualHold);
    pt.PutFloat("pull_stop_time", _p._pullStopTime);
    serial::SaveInNewChildOf(pt, "death_start_trigger", _p._deathStartTriggerEditorId); 
    serial::SaveInNewChildOf(pt, "death_end_trigger", _p._deathEndTriggerEditorId);

}

void FlowPlayerEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    _p._defaultLaunchVel = pt.GetFloat("launch_vel");
    pt.TryGetFloat("max_horiz_speed_after_dash", &_p._maxHorizSpeedAfterDash);
    pt.TryGetFloat("max_vert_speed_after_dash", &_p._maxVertSpeedAfterDash);
    pt.TryGetFloat("max_overall_speed_after_dash", &_p._maxOverallSpeedAfterDash);
    pt.TryGetFloat("dash_time", &_p._dashTime);
    serial::LoadFromChildOf(pt, "gravity", _p._gravity);
    pt.TryGetFloat("max_fall_speed", &_p._maxFallSpeed);
    pt.TryGetBool("pull_manual_hold", &_p._pullManualHold);
    pt.TryGetFloat("pull_stop_time", &_p._pullStopTime);
    serial::LoadFromChildOf(pt, "death_start_trigger", _p._deathStartTriggerEditorId);
    serial::LoadFromChildOf(pt, "death_end_trigger", _p._deathEndTriggerEditorId);
}

ne::BaseEntity::ImGuiResult FlowPlayerEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;

    ImGui::InputFloat("Launch vel", &_p._defaultLaunchVel);
    ImGui::InputFloat("Max vx after dash", &_p._maxHorizSpeedAfterDash);
    ImGui::InputFloat("Max vy after dash", &_p._maxVertSpeedAfterDash);
    ImGui::InputFloat("Max v after dash", &_p._maxOverallSpeedAfterDash);
    ImGui::InputFloat("Dash time", &_p._dashTime);
    imgui_util::InputVec3("gravity", &_p._gravity);
    ImGui::InputFloat("Max fall speed", &_p._maxFallSpeed);
    ImGui::Checkbox("Pull manual hold", &_p._pullManualHold);
    ImGui::InputFloat("Pull stop time", &_p._pullStopTime);
    imgui_util::InputEditorId("Death Start trigger", &_p._deathStartTriggerEditorId);
    imgui_util::InputEditorId("Death end trigger", &_p._deathEndTriggerEditorId); 

    return result;
}

void FlowPlayerEntity::InitDerived(GameManager& g) {
    _s = State();
    {
        ne::Entity* camera = g._neEntityManager->GetFirstEntityOfType(ne::EntityType::Camera);
        if (camera == nullptr) {
            printf("FlowPlayerEntity::InitDerived: couldn't find a camera entity!\n");
        } else {
            _s._cameraId = camera->_id;
        }        
    }
    _s._currentColor = _modelColor;
    _s._respawnPos = _initTransform.Pos();
    _transform.SetPos(_s._respawnPos);
    int constexpr kTailLength = 4;
    for (int ii = 0; ii < kTailLength; ++ii) {
        _s._tail.push_back({_transform.Pos(), Vec3()});
    }

    if (ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_p._deathStartTriggerEditorId, ne::EntityType::FlowTrigger)) {
        _s._deathStartTrigger = e->_id;
    }
    if (ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_p._deathEndTriggerEditorId, ne::EntityType::FlowTrigger)) {
        _s._deathEndTrigger = e->_id;
    }
    for (int ii = 0; ii < static_cast<size_t>(InputManager::Key::NumKeys); ++ii) {
        _s._haveSeenKeyUpSinceLastHit[ii] = true;
        _s._keyBufferTimeLeft[ii] = 0;
    }

    _s._misses.Clear();
}

namespace {

FlowWallEntity* FindOverlapWithWall(GameManager& g, Transform const& playerTrans, Vec3* penetrationOut) {
    ne::EntityManager::Iterator wallIter = g._neEntityManager->GetIterator(ne::EntityType::FlowWall);
    std::vector<Vec3> aabbPoly(4);
    for (; !wallIter.Finished(); wallIter.Next()) {
        FlowWallEntity* pWall = static_cast<FlowWallEntity*>(wallIter.GetEntity());
        if (!pWall->_canHit) {
            continue;
        }
        Transform const& wallTrans = pWall->_transform;
        Vec3 euler = wallTrans.Quat().EulerAngles();
        float yRot = -euler._y;
        Vec3 penetration;
        bool hasOverlap = false;
        if (pWall->_polygon.size() < 3) {
            Vec3 halfScale = wallTrans.Scale() * 0.5f + 0.5f * playerTrans.Scale();
            aabbPoly[0].Set(halfScale._x, 0.f, halfScale._z);
            aabbPoly[1].Set(halfScale._x, 0.f, -halfScale._z);
            aabbPoly[2].Set(-halfScale._x, 0.f, -halfScale._z);
            aabbPoly[3].Set(-halfScale._x, 0.f, halfScale._z);
            
            hasOverlap = geometry::PointInConvexPolygon2D(playerTrans.Pos(), aabbPoly, wallTrans.Pos(), yRot, Vec3(1.f, 1.f, 1.f), &penetration);
        } else {
            hasOverlap = geometry::PointInConvexPolygon2D(playerTrans.Pos(), pWall->_polygon, wallTrans.Pos(), yRot, wallTrans.Scale() + (playerTrans.Scale() /** 0.5f*/), &penetration);
        }
        if (hasOverlap) {
            *penetrationOut = penetration;
            return pWall;
        }
    }
    return nullptr;
}

void DrawTicks(int activeCount, int totalCount, Transform const& renderTrans, float scale, GameManager& g) {
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

void RespawnDraw(FlowPlayerEntity& e, GameManager& g) {
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double respawnTimeElapsed = beatTime - e._s._respawnStartBeatTime;
    double explodeStart = 0.5;
    double explodeTime = 2.5;
    float explodeFactor = (float)((respawnTimeElapsed - explodeStart) / explodeTime);
    Vec3 center = e._transform.Pos();
    if (explodeFactor > 0.5f) {
        // [0.5,1] --> [0,1]
        float moveFactor = explodeFactor * 2.f - 1.f;
        moveFactor = math_util::SmoothStep(moveFactor);
        center = math_util::Vec3Lerp(moveFactor, center, e._s._respawnPos);
    }
    explodeFactor = math_util::SmoothUpAndDown(explodeFactor);

    float constexpr kOffsets[] = { -1.f, 1.f };
    Quaternion explodeQ;
    explodeQ.SetFromEulerAngles(Vec3(35.f, 0.f, 35.f) * kDeg2Rad);
    
    float constexpr kInitRadPerBeat = 2 * kPi / 8.f;
    float constexpr kRotAccel = 4.f;
    float rot = 0.f + kInitRadPerBeat*respawnTimeElapsed + 0.5f*kRotAccel*respawnTimeElapsed*respawnTimeElapsed;
    Quaternion dQ;
    dQ.SetFromAngleAxis(rot, Vec3(0.f, 0.f, 1.f));
    explodeQ = dQ * explodeQ;

    Mat3 R;
    explodeQ.GetRotMat(R); 
    float explodeInitialDist = e._transform.Scale()(0) * 0.25f;
    float explodeMaxDist = 2.f;
    float explodeDist = math_util::Lerp(explodeInitialDist, explodeMaxDist, explodeFactor);
    
    Quaternion dQ2;
    //dQ2.SetFromAngleAxis(kRotRadPerBeat * 2.f * respawnTimeElapsed, Vec3(1.f, 0.f, 0.f));
    for (int ii = 0; ii < 2; ++ii) {
        for (int jj = 0; jj < 2; ++jj) {
            for (int kk = 0; kk < 2; ++kk) {
                Vec3 offset(kOffsets[ii], kOffsets[jj], kOffsets[kk]);
                offset = R * offset;
                offset *= explodeDist;
                Transform t;
                t.SetScale(e._transform.Scale() * 0.5f);
                t.SetQuat(dQ2 * explodeQ);
                t.SetPos(center + offset);
                renderer::ModelInstance& model = g._scene->DrawCube(t.Mat4Scale(), e._s._currentColor);
                model._topLayer = true;
            }
        }
    }
}

void ResetFlowSection(GameManager& g, int currentSectionId) {
    // deactivate all initially-inactive things
    for (ne::EntityManager::AllIterator iter = g._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity* e = iter.GetEntity();
        if (!e->_initActive && e->_flowSectionId >= 0 && e->_flowSectionId == currentSectionId) {
            g._neEntityManager->TagForDeactivate(e->_id);
        }
    }

    // Reset everything from the current section
    for (ne::EntityManager::AllIterator iter = g._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity* e = iter.GetEntity();
        if (e->_flowSectionId >= 0 && e->_flowSectionId == currentSectionId) {
            e->Init(g);
        }
    }

    // Reactivate all inactive entities that were initially active    
    for (ne::EntityManager::AllIterator iter = g._neEntityManager->GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity* e = iter.GetEntity();
        if (e->_initActive && e->_flowSectionId >= 0 && e->_flowSectionId == currentSectionId) {
            g._neEntityManager->TagForActivate(e->_id, /*initOnActivate=*/true);
        }
    }    
}

void RespawnPlayer(FlowPlayerEntity& p, GameManager& g) {
    p._transform.SetTranslation(p._s._respawnPos);
    for (FlowPlayerEntity::TailState& t : p._s._tail) {
        t.p = p._transform.Pos();
        t.v = Vec3();
    }
    p._s._moveState = FlowPlayerEntity::MoveState::Default;
    p._s._vel.Set(0.f,0.f,0.f);
    p._s._dashTimer = -1.f;
    p._s._respawnBeforeFirstInteract = true;
    p._s._respawnStartBeatTime = -1.0;

    // Activate the on-respawn entity
    if (ne::Entity* e = g._neEntityManager->GetEntity(p._s._toActivateOnRespawn, /*includeActive=*/false, /*includeInactive=*/true)) {
        g._neEntityManager->TagForActivate(e->_id, /*initOnActivate=*/true);
    }
    else if (ne::Entity* e = g._neEntityManager->GetEntity(p._s._toActivateOnRespawn)) {
        e->Init(g);
    }

    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
#if 0
    _s._countOffEndTime = BeatClock::GetNextBeatDenomTime(beatTime, _s._respawnLoopLength);
    int minCountOffTime = 3;
    if (_s._countOffEndTime - beatTime < minCountOffTime) {
        _s._countOffEndTime += _s._respawnLoopLength;
    }
#endif
    p._s._countOffEndTime = BeatClock::GetNextBeatDenomTime(beatTime + 4.0, 1.0);

}

}  // namespace

void FlowPlayerEntity::Draw(GameManager& g, float const dt) {

    if (_s._respawnStartBeatTime > 0.0) {
        RespawnDraw(*this, g);
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    
    if (_s._moveState == MoveState::Default) {        
        if (_s._countOffEndTime >= 0.0 && beatTime < _s._countOffEndTime) {
            double timeLeft = _s._countOffEndTime - beatTime;
            int numActive = (int)std::ceil(4 - timeLeft);
            numActive = std::max(0, numActive);
            DrawTicks(numActive, 4, _transform, 1.f, g); 
        }
    } 

    // Draw tail
    Mat4 tailTrans;
    tailTrans.Scale(0.08f, 0.08f, 0.08f);
    for (TailState const& t : _s._tail) {
        tailTrans.SetTranslation(t.p);
        renderer::ModelInstance& model = g._scene->DrawCube(tailTrans, _s._currentColor);
        model._topLayer = true;
    }

    Transform renderTrans = _transform;

    if (_s._moveState == MoveState::Pull || _s._moveState == MoveState::Push) {

        g._scene->DrawLine(_transform.Pos(), _s._lastKnownDashTarget, _s._lastKnownDashTargetColor);

        if (_s._vel.Length2() > 0.1f) {
            _s._dashAnimDir = _s._vel.GetNormalized();
        }
    }

    float constexpr kMaxSquishInTime = 0.1f;
    float constexpr kMaxSquishOutTime = 0.1f;    
    switch (_s._dashAnimState) {
    case DashAnimState::Accel: {
        float const kSquishDelta = dt / kMaxSquishInTime;
        _s._dashSquishFactor += kSquishDelta;
        if (_s._dashSquishFactor > 1.f) {
            _s._dashSquishFactor = 1.f;
            _s._dashAnimState = DashAnimState::Decel;
        }
    }
        break;
    case DashAnimState::Decel: {
        float const kSquishDelta = dt / kMaxSquishOutTime;
        _s._dashSquishFactor -= kSquishDelta;
        if (_s._dashSquishFactor < 0.f) {
            _s._dashSquishFactor = 0.f;
            _s._dashAnimState = DashAnimState::None;
        }
    }
        break;
    case DashAnimState::None:
        _s._dashSquishFactor = 0.f;
        break;
    }

    if (_s._dashSquishFactor > 0.f) {
        Vec3 newX = _s._dashAnimDir;
        Vec3 newY(0.f, 1.f, 0.f);
        Mat3 rotMat;
        rotMat.SetCol(0, newX);
        rotMat.SetCol(1, newY);
        Vec3 newZ = Vec3::Cross(newX, newY);
        rotMat.SetCol(2, newZ);
        Quaternion newQ;
        newQ.SetFromRotMat(rotMat);
        renderTrans.SetQuat(newQ);

        float constexpr kTangentSquishAtMaxSpeed = 5.f;
        float constexpr kOrthoSquishAtMinSpeed = 0.5f;
        float constexpr kOrthoSquishAtMaxSpeed = 0.5f;
        float constexpr kSpeedOfMaxSquish = 40.f;
        float speedFactor = _s._dashLaunchSpeed / kSpeedOfMaxSquish;
        float const kMaxTangentSquish = 1.f + speedFactor * (kTangentSquishAtMaxSpeed - 1.f);
        float const kMaxOrthoSquish = kOrthoSquishAtMinSpeed + speedFactor * (kOrthoSquishAtMaxSpeed - kOrthoSquishAtMinSpeed);
        float squishFactor = math_util::Clamp(_s._dashSquishFactor, 0.f, 1.f);
        float tangentSquish = 1.f + squishFactor * (kMaxTangentSquish - 1.f);
        float orthoSquish = 1.f + squishFactor * (kMaxOrthoSquish - 1.f);
        Vec3 squishScale(tangentSquish, orthoSquish, orthoSquish);
        renderTrans.ApplyScale(squishScale);
        float scaleDiffInMotionDir = renderTrans.Scale()._x - _transform.Scale()._x;
        Vec3 p = renderTrans.Pos();
        p -= _s._dashAnimDir * (0.5f * scaleDiffInMotionDir);
        renderTrans.SetPos(p);
    }

    if (_model != nullptr) {
        Vec4 color = _s._currentColor;
        if (_s._misses._count > 0) {
            double const t = beatTime - _s._misses[_s._misses._count - 1]->t;
            double constexpr kMissFadeTime = 0.5;
            float factor = (float)(math_util::InverseLerp(0.0, kMissFadeTime, t));
            color = math_util::Vec4Lerp(Vec4(1.f, 0.f, 0.f, 1.f), _s._currentColor, factor);
        }
        renderer::ModelInstance& model = g._scene->DrawMesh(_model, renderTrans.Mat4Scale(), color);
        model._topLayer = true;
    }

    // Draw misses (assumes ringbuffer is ordered by t)
    bool append = false;
    Transform textTrans = _transform;
    textTrans.Translate(Vec3(0.f, 0.f, -0.5f));
    for (int ii = 0; ii < _s._misses._count; ++ii) {
        double constexpr kMissFadeTime = 0.5;
        Miss const& m = *_s._misses[ii];
        double const t = beatTime - m.t;
        if (t >= kMissFadeTime) {
            _s._misses.Pop();
            --ii;
            continue;
        }
        float constexpr kTextSize = 1.5f;
        float factor = (float)(math_util::InverseLerp(0.0, kMissFadeTime, t));
        float alpha = math_util::Lerp(1.f, 0.f, factor);
        Vec4 color(1.f, 0.f, 0.f, alpha);
        std::string text(1, m.c);
        g._scene->DrawTextWorld(std::move(text), textTrans.Pos(), kTextSize, color, append);
        append = true;
    }
}

void FlowPlayerEntity::RespawnInstant(GameManager& g) {
    ResetFlowSection(g, _s._currentSectionId);
    RespawnPlayer(*this, g);
}

void FlowPlayerEntity::SetNewSection(GameManager& g, int newSectionId) {
    _s._currentSectionId = newSectionId;
}

namespace {
    bool InteractingWithEnemy(FlowPlayerEntity::MoveState s) {
        switch (s) {
        case FlowPlayerEntity::MoveState::Default: return false;
        case FlowPlayerEntity::MoveState::WallBounce: return false;
        case FlowPlayerEntity::MoveState::Pull: return true;
        case FlowPlayerEntity::MoveState::PullStop: return true;
        case FlowPlayerEntity::MoveState::Push: return true;
        case FlowPlayerEntity::MoveState::Carried: return true;
        }
        return false;
    }

float fast_negexp(float x)
{
    return 1.0f / (1.0f + x + 0.48f*x*x + 0.235f*x*x*x);
}
float halflife_to_damping(float halflife, float eps = 1e-5f)
{
    return (4.0f * 0.69314718056f) / (halflife + eps);
}
void simple_spring_damper_exact(
    float& x, 
    float& v, 
    float x_goal, 
    float halflife, 
    float dt) {
    float y = halflife_to_damping(halflife) / 2.0f;	
    float j0 = x - x_goal;
    float j1 = v + j0*y;
    float eydt = fast_negexp(y*dt);

    x = eydt*(j0 + j1*dt) + x_goal;
    v = eydt*(v - j1*y*dt);
}

bool IsActionOnly(TypingEnemyEntity const& e) {
    if (e._p._hitResponse._type == HitResponseType::None) {
        if (!e._p._useLastHitResponse || e._p._lastHitResponse._type == HitResponseType::None) {
            return true;
        }
    }
    return false;
}
}

void FlowPlayerEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    for (int ii = 0; ii < static_cast<size_t>(InputManager::Key::NumKeys); ++ii) {
        if (g._inputManager->IsKeyPressedThisFrame(static_cast<InputManager::Key>(ii))) {
            _s._keyBufferTimeLeft[ii] = 2;  // frames
        } else if (_s._keyBufferTimeLeft[ii] > 0) {
            --_s._keyBufferTimeLeft[ii];
        }

        if (!g._inputManager->IsKeyPressed(static_cast<InputManager::Key>(ii))) {
            _s._haveSeenKeyUpSinceLastHit[ii] = true;
        }
    }
 
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (_s._respawnStartBeatTime > 0.0) {
        //double respawnEndTime = g._beatClock->GetNextDownBeatTime(_s._respawnStartBeatTime + 4.0);
        if (!_s._respawnHasResetScene) {
            double resetSceneTime = _s._respawnStartBeatTime + 1.25;
            if (beatTime >= resetSceneTime) {
                ResetFlowSection(g, _s._currentSectionId);
                _s._respawnHasResetScene = true;
            }
        }
        double respawnEndTime = _s._respawnStartBeatTime + 3.5;
        if (beatTime >= respawnEndTime) {
            if (FlowTriggerEntity* e = g._neEntityManager->GetEntityAs<FlowTriggerEntity>(_s._deathEndTrigger)) {
                e->OnTrigger(g);
            }
            RespawnPlayer(*this, g);
            return;
        }
        return;
    }

    if (_s._countOffEndTime >= 0.0 && beatTime < _s._countOffEndTime) {
        double unusedBeatNum;
        float beatTimeFrac = (float) (modf(beatTime, &unusedBeatNum));
        // 0 is default color, 1 is temp flash color
        float factor = math_util::SmoothStep(beatTimeFrac);
        Vec4 constexpr kFlashColor(0.f, 0.f, 0.f, 1.f);
        _s._currentColor = _modelColor + factor * (kFlashColor - _modelColor);

        double timeLeft = _s._countOffEndTime - beatTime;
        if (timeLeft > 0.24) {
            return;
        } 
    } else {
        _s._currentColor = _modelColor;
    }

    //
    // Perform release actions
    //
    {
        int ixOfLastElement = _s._heldActions.size() - 1;
        for (int ii = 0; ii <= ixOfLastElement; ++ii) {
            HeldAction& a = _s._heldActions[ii];
            if (!g._inputManager->IsKeyPressed(a._key)) {
                for (auto& action : a._actions) {
                    action->ExecuteRelease(g);
                }
                if (ii < ixOfLastElement) {
                    HeldAction& lastElement = _s._heldActions[ixOfLastElement];
                    std::swap(a, lastElement);
                    --ii;  // stay on the same ix on the next loop iteration
                }
                --ixOfLastElement;
            }
        }
        _s._heldActions.resize(ixOfLastElement + 1);
    }

    // update carried status
    if (_s._moveState != MoveState::Carried) { 
        for (ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !enemyIter.Finished(); enemyIter.Next()) {
            TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
            if (!enemy->_p._lockPlayerOnEnterRegion) {
                continue;
            }
            
            if (g._neEntityManager->GetEntity(enemy->_s._activeRegionId) && enemy->CanHit(*this, g)) {
                _s._moveState = MoveState::Carried;
                _s._carrierId = enemy->_id;
                _s._dashTimer = -1.f;
                _s._dashAnimState = DashAnimState::None;
                _s._vel.Set(0.f, 0.f, 0.f);
                break;
            } 
        }
    }

    // Check if player has pressed any key
    bool playerPressedKey = false;
    char pressedKey = 'a';
    for (int ii = (int)InputManager::Key::A; ii <= (int)InputManager::Key::Z; ++ii) {
        InputManager::Key k = static_cast<InputManager::Key>(ii);
        if (g._inputManager->IsKeyPressedThisFrame(k)) {
            playerPressedKey = true;
            pressedKey = 'a' + (ii - (int)InputManager::Key::A);
            break;
        }
    }
 
    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    Vec3 const& playerPos = _transform.GetPos();    
    Mat4 viewProjTransform = g._scene->GetViewProjTransform();
    bool const usingController = g._inputManager->IsUsingController();
    InputManager::Key hitKey = InputManager::Key::NumKeys;
    int hitKeyIndex = -1;
    TypingEnemyEntity* carrier = nullptr;
    if (_s._moveState == MoveState::Carried) {
        carrier = g._neEntityManager->GetEntityAs<TypingEnemyEntity>(_s._carrierId);
    }
    for (ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !enemyIter.Finished(); enemyIter.Next()) {
        // Vec4 constexpr kGreyColor(0.6f, 0.6f, 0.6f, 0.7f);
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();

        // TODO this is inefficient I know, shutup
        if (carrier != nullptr && carrier->_id != enemy->_id && !IsActionOnly(*enemy)) {
            continue;
        }
        
        Vec3 dp = playerPos - enemy->_transform.GetPos();
        dp._y = 0.f;
        float d2 = dp.Length2();
        if (!enemy->CanHit(*this, g)) {
            continue;
        }
        
        // bool clear = IsCollisionFree(g, playerPos, enemy->_transform.GetPos());
        // if (!clear) {
        //     enemy->_currentColor = kGreyColor;
        //     enemy->_textColor = kGreyColor;
        //     continue;
        // }
        // enemy->_s._currentColor = enemy->_modelColor;        

        bool hasHit = false;
        if (usingController) {
            InputManager::ControllerButton nextButton = enemy->GetNextButton();
            // TODO: plug in haveSeenKeyUp whatever
            if (nextButton == InputManager::ControllerButton::Count || !g._inputManager->IsKeyPressedThisFrame(nextButton)) {
                continue;
            }
        } else {
            TypingEnemyEntity::NextKeys nextKeys = enemy->GetNextKeys();
            for (int kIx = 0; kIx < nextKeys.size(); ++kIx) {
                InputManager::Key key = nextKeys[kIx];
                if (key == InputManager::Key::NumKeys) {
                    break;
                } 
                bool hit = g._inputManager->IsKeyPressed(key) && _s._haveSeenKeyUpSinceLastHit[static_cast<size_t>(key)] && _s._keyBufferTimeLeft[static_cast<size_t>(key)] > 0;
                if (hit) {
                    hasHit = true;
                    hitKey = key;
                    hitKeyIndex = kIx;
                    break;
                }
            } 
        }

        if (!hasHit) {
            continue;
        }

        if (enemy->_id == _s._lastHitEnemy) {
            // Check if it's in view of camera
            float screenX, screenY;
            geometry::ProjectWorldPointToScreenSpace(enemy->_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);
            if (screenX < 0 || screenX > g._windowWidth || screenY < 0 || screenY > g._windowHeight) {
                continue;
            }
            nearest = enemy;
            nearestDist2 = d2;
            break;
        }

        if (nearest == nullptr || d2 < nearestDist2) {

            // Check if it's in view of camera
            float screenX, screenY;
            geometry::ProjectWorldPointToScreenSpace(enemy->_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);
            if (screenX < 0 || screenX > g._windowWidth || screenY < 0 || screenY > g._windowHeight) {
                continue;
            }
            
            nearest = enemy;
            nearestDist2 = d2;
        }
    }

    if (nearest) {
        _s._haveSeenKeyUpSinceLastHit[static_cast<size_t>(hitKey)] = false;
        if (hitKeyIndex < 0) {
            printf("FlowPlayer: WTF nearest without hitKeyIndex?\n");
        }
    }

    if (playerPressedKey && nearest == nullptr) {
        // Trigger miss actions
        FlowTriggerEntity* e = g._neEntityManager->GetEntityAs<FlowTriggerEntity>(_s._missTrigger);
        if (e) {
            e->OnTrigger(g);
        }
        

        Miss* m = _s._misses.Push();
        if (m != nullptr) {
            m->c = pressedKey;
            m->t = beatTime;
        }
    }

    //
    // Check if we start a new enemy interaction
    //
    bool actionOnly = false;
    if (nearest != nullptr) {
        actionOnly = IsActionOnly(*nearest);
        if (actionOnly) {
            nearest->OnHit(g, hitKeyIndex);
        }
    }
    if (nearest != nullptr && !actionOnly) {
        _s._respawnBeforeFirstInteract = false;
        
        HitResult hitResult = nearest->OnHit(g, hitKeyIndex);
        HitResponse const& hitResponse = hitResult._response;

        HeldAction& heldAction = _s._heldActions.emplace_back();
        heldAction._key = hitKey;
        heldAction._actions = std::move(hitResult._heldActions);

        _s._lastHitEnemy = nearest->_id;
        _s._heldKey = hitKey;

        //if (_s._moveState == MoveState::Carried) {
        //    nearest->Bump(Vec3(0.f, 0.f, 1.f));
        //} else {
            
            switch (hitResponse._type) {
                case HitResponseType::None:
                    nearest->Bump(Vec3(0.f, 0.f, 1.f));
                    break;
                case HitResponseType::Pull: {
                    _s._interactingEnemyId = nearest->_id;
                    _s._stopAfterTimer = hitResponse._stopAfterTimer;
                    _s._stopDashOnPassEnemy = hitResponse._stopOnPass;
                    // If we are close enough to the enemy, just stick to them and start in PullStop if applicable.
                    if (_s._stopDashOnPassEnemy && nearestDist2 < 0.001f * 0.001f) {
                        _transform.SetPos(nearest->_transform.Pos());
                        _s._moveState = MoveState::PullStop;
                        _s._passedPullTarget = true;
                        _s._dashAnimState = DashAnimState::None;
                    } else {
                        _s._moveState = MoveState::Pull;
                        _s._passedPullTarget = false;
                        _s._dashAnimState = DashAnimState::Accel;
                    }
                    _s._dashTimer = 0.f;                        
                    if (hitResponse._speed >= 0.f) {
                        _s._dashLaunchSpeed = hitResponse._speed;
                    } else {
                        _s._dashLaunchSpeed = _p._defaultLaunchVel;
                    }
                    Vec3 playerToTargetDir = nearest->_transform.Pos() - playerPos;
                    float dist = playerToTargetDir.Normalize();
                    _s._vel = playerToTargetDir * _s._dashLaunchSpeed;
                    if (dist < 0.001f) {
                        playerToTargetDir.Set(1.f, 0.f, 0.f);
                    }
                    nearest->Bump(playerToTargetDir);
                }
                    break;
                case HitResponseType::Push: {
                    _s._interactingEnemyId = nearest->_id;
                    _s._moveState = MoveState::Push;
                    _s._stopAfterTimer = hitResponse._stopAfterTimer;
                    _s._dashTimer = 0.f;
                    _s._dashAnimState = DashAnimState::Accel;
                    Vec3 pushDir;
                    if (hitResponse._pushAngleDeg >= 0.f) {
                        float angleRad = hitResponse._pushAngleDeg * kDeg2Rad;
                        pushDir._x = cos(angleRad);
                        pushDir._y = 0.f;
                        pushDir._z = -sin(angleRad);
                    } else {
                        pushDir = playerPos - nearest->_transform.GetPos();
                        pushDir._y = 0.f;
                        pushDir.Normalize();
                    }
                    if (hitResponse._speed >= 0.f) {
                        _s._dashLaunchSpeed = hitResponse._speed;
                    } else {
                        _s._dashLaunchSpeed = _p._defaultLaunchVel;
                    }
                    _s._vel = pushDir * _s._dashLaunchSpeed;
                    nearest->Bump(pushDir);
                }
                    break;
                case HitResponseType::Count: {
                    assert(false);
                    break;
                }
            }
        //}

        ne::EntityId nearestId = nearest->_id;
        for (auto enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !enemyIter.Finished(); enemyIter.Next()) {
            TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
            if (enemy->_id != nearestId) {
                enemy->OnHitOther(g);
            }
        }
    }

    //
    // Check if we're done with an existing interaction
    //
    switch (_s._moveState) {
        case MoveState::Default: break;
        case MoveState::Pull: {
            bool finishedPulling = false;
            if (_p._pullManualHold && _s._heldKey != InputManager::Key::NumKeys && g._inputManager->IsKeyReleasedThisFrame(_s._heldKey)) {
                finishedPulling = true;
                _s._heldKey = InputManager::Key::NumKeys;
            } else if (_s._stopAfterTimer && _s._dashTimer >= _p._dashTime) {
                finishedPulling = true;
            }
            if (finishedPulling) {
                _s._moveState = MoveState::Default;
                _s._interactingEnemyId = ne::EntityId();
                if (_p._maxHorizSpeedAfterDash >= 0.f) {
                    _s._vel._x = math_util::ClampAbs(_s._vel._x, _p._maxHorizSpeedAfterDash);
                }
                if (_p._maxVertSpeedAfterDash >= 0.f) {
                    _s._vel._z = math_util::ClampAbs(_s._vel._z, _p._maxVertSpeedAfterDash);
                }
                if (_p._maxOverallSpeedAfterDash >= 0.f) {
                    float speed = _s._vel.Normalize();
                    speed = std::min(_p._maxOverallSpeedAfterDash, speed);
                    _s._vel *= speed;
                }
            }
        }
        break;

        case MoveState::PullStop: {
            if (_s._dashTimer >= _p._pullStopTime) {
                _s._moveState = MoveState::Default;
                if (TypingEnemyEntity* e = g._neEntityManager->GetEntityAs<TypingEnemyEntity>(_s._interactingEnemyId)) {
                    e->DoComboEndActions(g);
                }
                _s._interactingEnemyId = ne::EntityId();
            }
        }
        break;

        case MoveState::Push: {
            bool finishedPushing = false;
            if (_p._pullManualHold && _s._heldKey != InputManager::Key::NumKeys && g._inputManager->IsKeyReleasedThisFrame(_s._heldKey)) {
                finishedPushing = true;
                _s._heldKey = InputManager::Key::NumKeys;
            } else if (_s._stopAfterTimer && _s._dashTimer >= _p._dashTime) {
                finishedPushing = true;
            }
            if (finishedPushing) {
                _s._moveState = MoveState::Default;
                if (_p._maxHorizSpeedAfterDash >= 0.f) {
                    _s._vel._x = math_util::ClampAbs(_s._vel._x, _p._maxHorizSpeedAfterDash);
                }
                if (_p._maxVertSpeedAfterDash >= 0.f) {
                    _s._vel._z = math_util::ClampAbs(_s._vel._z, _p._maxVertSpeedAfterDash);
                }
                if (_p._maxOverallSpeedAfterDash >= 0.f) {
                    float speed = _s._vel.Normalize();
                    speed = std::min(_p._maxOverallSpeedAfterDash, speed);
                    _s._vel *= speed;
                }
            }
        }
        break;

        case MoveState::WallBounce: {
            // TODO make this a separate property!
            float const wallBounceTime = 0.5f * _p._dashTime;
            if (_s._dashTimer >= wallBounceTime) {
                _s._moveState = MoveState::Default;
                // TODO: Should bounce transition be the same as pull's?
                if (_p._maxHorizSpeedAfterDash >= 0.f) {
                    _s._vel._x = math_util::ClampAbs(_s._vel._x, _p._maxHorizSpeedAfterDash);
                }
                if (_p._maxVertSpeedAfterDash >= 0.f) {
                    _s._vel._z = math_util::ClampAbs(_s._vel._z, _p._maxVertSpeedAfterDash);
                }
                if (_p._maxOverallSpeedAfterDash >= 0.f) {
                    float speed = _s._vel.Normalize();
                    speed = std::min(_p._maxOverallSpeedAfterDash, speed);
                    _s._vel *= speed;
                }
            }
            break;
        }

        case MoveState::Carried: {
            if (carrier == nullptr) {
                _s._moveState = MoveState::Default;
            }

            break;
        }
        break;
    }

    //
    // Update position/velocity/etc based on current state
    //
    Vec3 const previousPos = _transform.Pos();
    Vec3 newPos = previousPos;
    if (InteractingWithEnemy(_s._moveState)) {
        if (TypingEnemyEntity* dashTarget = g._neEntityManager->GetEntityAs<TypingEnemyEntity>(_s._interactingEnemyId)) {
            _s._lastKnownDashTarget = dashTarget->_transform.Pos();
            _s._lastKnownDashTarget._y = playerPos._y;
            _s._lastKnownDashTargetColor = dashTarget->_s._currentColor;
            _s._lastKnownDashTargetColor._w = 1.f;
        }
    }
    switch (_s._moveState) {
        case MoveState::Default: {
            if (!_s._respawnBeforeFirstInteract) {
                _s._vel += _p._gravity * dt;
                if (_p._maxFallSpeed >= 0.f) {
                    _s._vel._z = std::min(_p._maxFallSpeed, _s._vel._z);
                }
            }
            newPos += _s._vel * dt;
        }
        break;
        case MoveState::WallBounce: {
            _s._dashTimer += dt;
            newPos += _s._vel * dt;            
        }
        break;
        case MoveState::Pull: {
            _s._dashTimer += dt;
            if (_s._passedPullTarget) {
                newPos += _s._vel * dt;
            } else {
                Vec3 playerToTargetDir = _s._lastKnownDashTarget - previousPos;
                playerToTargetDir.Normalize();
                float currentSpeed = _s._vel.Length();
                _s._vel = playerToTargetDir * currentSpeed;
                newPos += _s._vel * dt;
                Vec3 newPlayerToTarget = _s._lastKnownDashTarget - newPos;
                float dotP = Vec3::Dot(newPlayerToTarget, _s._vel);
                if (dotP < 0.f) {
                    // we passed the dash target
                    _s._passedPullTarget = true;
                    if (_s._stopDashOnPassEnemy) {
                        _s._vel.Set(0.f, 0.f, 0.f);
                        newPos = _s._lastKnownDashTarget;
                        _s._moveState = MoveState::PullStop;
                        _s._dashTimer = 0.f;
                    }
                }
            }
        }
        break;
        case MoveState::PullStop: {
            _s._dashTimer += dt;
        }
        break;
        case MoveState::Push: {
            _s._dashTimer += dt;
            newPos += _s._vel * dt;
        }
        break;
        case MoveState::Carried: {
            assert(carrier != nullptr);
            newPos = carrier->_transform.Pos();
        }
        break;        
    }

    _transform.SetPos(newPos);


    // Check if we've hit a wall
    //
    // TODO: Should we run this after respawn logic?
    {
        Vec3 penetration;
        FlowWallEntity* pHitWall = FindOverlapWithWall(g, _transform, &penetration);
        penetration._y = 0.f;
        if (pHitWall != nullptr) {
            pHitWall->OnHit(g, -penetration.GetNormalized());
            if (pHitWall->_hurtOnHit) {
                StartRespawn(g);
            } else {
                _s._moveState = MoveState::WallBounce;
                _s._interactingEnemyId = ne::EntityId();
                newPos += penetration * 1.1f;  // Add some padding to ensure we're outta collision
                Vec3 collNormal = penetration.GetNormalized();
                Vec3 newVel = -_s._vel;
                // reflect across coll normal
                Vec3 tangentPart = Vec3::Dot(newVel, collNormal) * collNormal;
                Vec3 normalPart = newVel - tangentPart;
                newVel -= 2 * normalPart;        

                // after bounce, ensure we have some velocity in the collision normal
                // float constexpr kMinBounceSpeed = 20.f;
                // float speedAlongNormal = Vec3::Dot(newVel, collNormal);
                // if (speedAlongNormal < kMinBounceSpeed) {
                //     Vec3 correction = (kMinBounceSpeed - speedAlongNormal) * collNormal;
                //     newVel += correction;
                // }
                float constexpr kBounceSpeed = 5.f;
                newVel = tangentPart.GetNormalized() * kBounceSpeed;

                _transform.SetTranslation(newPos);
                _s._vel = newVel;
                _s._dashTimer = 0.f;
                _s._dashAnimState = DashAnimState::None;
            }
        }
    }
    

    // Update tail
    Vec3 desiredPos = _transform.Pos();
    float halfLife = 0.025f;
    if (_s._moveState == MoveState::Carried) {
        static Vec3 lastDesiredPos;
        float constexpr kWiggleAmp = 0.1f;
        Vec3 motionDir = desiredPos - lastDesiredPos;
        motionDir.Normalize();
        Vec3 offsetDir = Vec3::Cross(motionDir, Vec3(0.f, 1.f, 0.f));
        double beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double beatFrac = BeatClock::GetBeatFraction(beatTime);
        float verticalOffset = kWiggleAmp * sin(2*kPi*beatFrac);
        desiredPos += offsetDir * verticalOffset;
        halfLife *= 2.f;

        lastDesiredPos = _transform.Pos();
    }
    for (int ii = 0; ii < _s._tail.size(); ++ii) {
        TailState& tailPoint = _s._tail[ii];
        for (int jj = 0; jj < 3; ++jj) {
            simple_spring_damper_exact(tailPoint.p(jj), tailPoint.v(jj), desiredPos(jj), halfLife, dt);
        } 
        desiredPos = tailPoint.p;
    } 

    // Check if we hit any pickups
    ne::EntityManager::Iterator pickupIter = g._neEntityManager->GetIterator(ne::EntityType::FlowPickup);
    for (; !pickupIter.Finished(); pickupIter.Next()) {
        FlowPickupEntity* pPickup = static_cast<FlowPickupEntity*>(pickupIter.GetEntity());
        assert(pPickup != nullptr);
        Transform const& pickupTrans = pPickup->_transform;
        Vec3 penetration;
        bool hasOverlap = geometry::DoAABBsOverlap(_transform, pickupTrans, &penetration);
        if (hasOverlap) {
            pPickup->OnHit(g);
        }
    }

    if (_s._killMaxZ.has_value()) {
        Vec3 p = _transform.Pos();
        if (p._z > _s._killMaxZ.value()) {
            StartRespawn(g);
            return;
        }
    }
    if (_s._killIfBelowCameraView || _s._killIfLeftOfCameraView) {
        Vec3 p = _transform.Pos();
        float playerSize = _transform.Scale()._x;
        float screenX, screenY;
        geometry::ProjectWorldPointToScreenSpace(p, viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);
        if (_s._killIfBelowCameraView && screenY > g._windowHeight + 4 * playerSize) {
            StartRespawn(g);
        } else if (_s._killIfLeftOfCameraView && screenX < -4 * playerSize) {
            StartRespawn(g);
        }
    }
}
