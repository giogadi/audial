#include "entities/sink.h"

#include <imgui/imgui.h>

#include "game_manager.h"
#include "renderer.h"
#include "beat_clock.h"
#include "input_manager.h"
#include "entities/resource.h"
#include "geometry.h"

void SinkEntity::SaveDerived(serial::Ptree pt) const {
    _p.key.Save(pt, "key"); 
    pt.PutDouble("quantize", _p.quantize);
    SeqAction::SaveActionsInChildNode(pt, "actions", _p.actions);
    SeqAction::SaveActionsInChildNode(pt, "accept_actions", _p.acceptActions);
}

void SinkEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    _p.key.Load(pt, "key");
    pt.TryGetDouble("quantize", &_p.quantize);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p.actions);
    SeqAction::LoadActionsFromChildNode(pt, "accept_actions", _p.acceptActions);
}

ne::BaseEntity::ImGuiResult SinkEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (_p.key.ImGui("Key")) {
        result = ImGuiResult::NeedsInit;
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
    bool keyDown = false;
    bool quantizeReached = false;
    double quantizedBeatTime = -1.0;
    InputManager::Key k = InputManager::CharToKey(_p.key.c);
    keyJustPressed = g._inputManager->IsKeyPressedThisFrame(k);
    keyJustReleased = g._inputManager->IsKeyReleasedThisFrame(k);
    keyDown = g._inputManager->IsKeyPressed(k);
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
    Vec3 p = _transform.Pos();
    Vec3 const& scale = _transform.Scale();
    p._y += 0.5f * scale._y;

    Transform t = _transform;
    t.SetPos(p);
    g._scene->DrawCube(t.Mat4Scale(), _modelColor);

    {
        float constexpr kTextSize = 1.5f;
        Vec4 color(1.f, 1.f, 1.f, 1.f);
        char textBuf[] = "*";
        textBuf[0] = _p.key.c;
        // AHHHH ALLOCATION
        g._scene->DrawTextWorld(std::string(textBuf), _transform.Pos(), kTextSize, color);
    }
 
}

void SinkEntity::OnEditPick(GameManager& g) {
}
