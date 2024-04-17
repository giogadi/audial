#include "entities/spawner.h"

#include <imgui/imgui.h>

#include "game_manager.h"
#include "entities/resource.h"
#include "beat_clock.h"
#include "input_manager.h"
#include "geometry.h"
#include "renderer.h"

void SpawnerEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "button", _p.btn);
    pt.PutDouble("quantize", _p.quantize);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p.actions);
}

void SpawnerEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    serial::LoadFromChildOf(pt, "button", _p.btn);
    pt.TryGetDouble("quantize", &_p.quantize);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p.actions);
}

ne::BaseEntity::ImGuiResult SpawnerEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (ImGui::TreeNode("Button")) {
        if (_p.btn.ImGui()) {
            result = ImGuiResult::NeedsInit;
        }
        ImGui::TreePop();
    }
    if (ImGui::InputDouble("Quantize", &_p.quantize)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("Actions", _p.actions)) {
        result = ImGuiResult::NeedsInit;
    }

    return result;
}

void SpawnerEntity::InitDerived(GameManager& g) {
    _s = State();
    for (auto const& pAction : _p.actions) {
        pAction->Init(g);
    }

}

void SpawnerEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    bool keyJustPressed = false;
    bool keyJustReleased = false;
    bool quantizeReached = false;
    double quantizedBeatTime = -1.0;
    InputManager::Key k = InputManager::CharToKey(_p.btn.key.c);
    keyJustPressed = g._inputManager->IsKeyPressedThisFrame(k);
    keyJustReleased = g._inputManager->IsKeyReleasedThisFrame(k);
    _s.keyPressed = g._inputManager->IsKeyPressed(k);
    {
        float keyPushFactorTarget = _s.keyPressed ? 1.f : 0.f;
        _s.keyPushFactor += dt * 32.f * (keyPushFactorTarget - _s.keyPushFactor);
    }
    if (keyJustPressed && _s.actionBeatTime < 0.0) {
        _s.actionBeatTime = g._beatClock->GetNextBeatDenomTime(beatTime, _p.quantize);
    }

    if (_s.actionBeatTime >= 0.0 && beatTime >= _s.actionBeatTime) {
        quantizeReached = true;
        quantizedBeatTime = _s.actionBeatTime;
        _s.actionBeatTime = -1.0;
    }
  
    if (quantizeReached) {

        for (auto const& pAction : _p.actions) {
            pAction->Execute(g);
        }

        // Check if there are any resources already in the spawner
        bool resourcePresent = false;
        for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
            ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
            assert(r);
            bool inRange = geometry::IsPointInBoundingBox2d(r->_transform.Pos(), _transform);
            if (inRange) {
                resourcePresent = true;
                break;
            }
        }
        if (!resourcePresent) {
            ResourceEntity* r = static_cast<ResourceEntity*>(g._neEntityManager->AddEntity(ne::EntityType::Resource));
            r->_initTransform = _transform;
            r->_initTransform.Translate(Vec3(0.f, 1.f, 0.f));
            r->_initTransform.SetScale(Vec3(0.25f, 0.25f, 0.25f));
            r->_modelName = "cube";
            r->_modelColor.Set(1.f, 1.f, 1.f, 1.f);

            r->Init(g);
            r->_s.expandTimeElapsed = 0.f;
        }
    }

}

void SpawnerEntity::Draw(GameManager& g, float dt) {
    Transform cubeCtrToBtm;
    cubeCtrToBtm.SetPos(Vec3(0.f, 0.5f, 0.f));
    Mat4 toBtmM = cubeCtrToBtm.Mat4NoScale();

    float constexpr kSinkHeight = 0.1f;
    Vec3 sinkRenderScale = _transform.Scale();
    sinkRenderScale._y = kSinkHeight;
    Transform sinkRenderT;
    sinkRenderT = _transform;
    sinkRenderT.SetScale(sinkRenderScale);
    Mat4 sinkRenderM = sinkRenderT.Mat4Scale() * toBtmM;
    g._scene->DrawCube(sinkRenderM, _modelColor);

    Transform btnTrans = _transform;
    btnTrans.Translate(Vec3(0.f, kSinkHeight, 0.f));
    _p.btn.Draw(g, btnTrans, _s.keyPushFactor, _s.keyPressed);

}

void SpawnerEntity::OnEditPick(GameManager& g) {
}
