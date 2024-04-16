#include "entities/sink.h"

#include <imgui/imgui.h>

#include "game_manager.h"
#include "renderer.h"
#include "beat_clock.h"
#include "input_manager.h"
#include "entities/resource.h"
#include "geometry.h"
#include "imgui_util.h"
#include "math_util.h"
#include "color.h"

void SinkEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "key", _p.key);
    pt.PutDouble("quantize", _p.quantize);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p.actions);
    SeqAction::SaveActionsInChildNode(pt, "accept_actions", _p.acceptActions);
}

void SinkEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    serial::LoadFromChildOf(pt, "key", _p.key);
    pt.TryGetDouble("quantize", &_p.quantize);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p.actions);
    SeqAction::LoadActionsFromChildNode(pt, "accept_actions", _p.acceptActions);
}

ne::BaseEntity::ImGuiResult SinkEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (ImGui::TreeNode("Key")) {
        if (_p.key.ImGui()) {
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
    if (SeqAction::ImGui("Accept Actions", _p.acceptActions)) {
        result = ImGuiResult::NeedsInit;
    }
 
    return ImGuiResult::Done;
}

void SinkEntity::InitDerived(GameManager& g) {
    _s = State();
    for (auto const& pAction : _p.actions) {
        pAction->Init(g);
    }
    for (auto const& pAction : _p.acceptActions) {
        pAction->Init(g);
    }
}

void SinkEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();

    bool keyJustPressed = false;
    bool keyJustReleased = false;
    bool quantizeReached = false;
    double quantizedBeatTime = -1.0;
    InputManager::Key k = InputManager::CharToKey(_p.key.key.c);
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
    }

    if (!keyJustPressed) {
        return;
    }
    for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
        ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
        assert(r);
        bool inRange = geometry::IsPointInBoundingBox2d(r->_transform.Pos(), _transform);
        if (!inRange) {
            continue;
        }

        g._neEntityManager->TagForDestroy(r->_id);
    }
}

void SinkEntity::Draw(GameManager& g, float dt) {
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
    _p.key.Draw(g, btnTrans, _s.keyPushFactor, _s.keyPressed);
}

void SinkEntity::OnEditPick(GameManager& g) {
}
