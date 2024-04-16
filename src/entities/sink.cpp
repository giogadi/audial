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

void MechKey::Save(serial::Ptree pt) const {
    key.Save(pt, "key");
    serial::SaveInNewChildOf(pt, "offset", offset);
}

void MechKey::Load(serial::Ptree pt) {
    key.Load(pt, "key");
    serial::LoadFromChildOf(pt, "offset", offset);
}

bool MechKey::ImGui() {
    bool changed = false;
    if (key.ImGui("Key")) {
        changed = true;
    }
    if (imgui_util::InputVec3("Key offset", &offset)) {
        changed = true;
    }
    return changed;
}

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
   
    Vec4 keyNominalColor;
    float minV, maxV;
    if (_s.keyPressed) {
        keyNominalColor.Set(0.0f, 1.f, 1.f, 1.f);
        minV = 0.8f;
        maxV = 0.8f;
    } else {
        keyNominalColor.Set(1.f, 1.f, 1.f, 1.f);
        minV = 0.85f;
        maxV = 1.f;
    } 
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    float colorFactor = g._beatClock->GetBeatFraction(beatTime);
    colorFactor = -math_util::Triangle(colorFactor) + 1.f;
    colorFactor = math_util::SmoothStep(colorFactor);
    Vec4 textHsva = RgbaToHsva(keyNominalColor);
    float v = math_util::Lerp(minV, maxV, colorFactor);
    textHsva._z = v;
    Vec4 textColor = HsvaToRgba(textHsva);

    renderer::BBox2d textBbox;
    Transform textTrans;
    char textBuf[] = "*";
    textBuf[0] = _p.key.key.c;
    size_t const textId = g._scene->DrawText3d(textBuf, 1, textTrans.Mat4Scale(), textColor, &textBbox); 
    Vec3 textOriginToCenter;
    textOriginToCenter._x = 0.5f * (textBbox.minX + textBbox.maxX);
    textOriginToCenter._y = 0.f;
    textOriginToCenter._z = 0.5f * (textBbox.minY + textBbox.maxY);
    Vec3 textExtents;
    textExtents._x = textBbox.maxX - textBbox.minX;
    textExtents._y = 0.f;
    textExtents._z = textBbox.maxY - textBbox.minY;

    float constexpr kSinkHeight = 0.1f;
    Vec3 sinkRenderScale = _transform.Scale();
    sinkRenderScale._y = kSinkHeight;
    Transform sinkRenderT;
    sinkRenderT = _transform;
    sinkRenderT.SetScale(sinkRenderScale);
    Mat4 sinkRenderM = sinkRenderT.Mat4Scale() * toBtmM;
    g._scene->DrawCube(sinkRenderM, _modelColor);


    float constexpr kButtonMaxHeight = 0.4f;
    float constexpr kButtonPushedHeight = 0.1f;
    float const btnHeight = math_util::Lerp(kButtonMaxHeight, kButtonPushedHeight, _s.keyPushFactor);

    Vec4 btnColor(245.f / 255.f, 187.f / 255.f, 49.f / 255.f, 1.f);
    Transform eToButtonT;
    Vec3 p = _p.key.offset + Vec3(0.f, kSinkHeight, 0.f);
    eToButtonT.SetPos(p);
    Vec3 btnScale = textExtents * 1.5f;
    btnScale._y = btnHeight;
    eToButtonT.SetScale(btnScale);
    Mat4 btnRenderM = _transform.Mat4NoScale() * eToButtonT.Mat4Scale() * toBtmM;
    g._scene->DrawCube(btnRenderM, btnColor);
    
    Transform btnToText;
    Vec3 btnToTextP = -textOriginToCenter + Vec3(0.f, btnHeight + 0.001f, 0.f);
    btnToText.SetPos(btnToTextP);
    Mat4 textRenderM = _transform.Mat4NoScale() * eToButtonT.Mat4NoScale() * btnToText.Mat4NoScale();
    renderer::Glyph3dInstance& glyph3d = g._scene->GetText3d(textId);
    glyph3d._t = textRenderM;
    
}

void SinkEntity::OnEditPick(GameManager& g) {
}
