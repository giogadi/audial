#include "mech_button.h"

#include <imgui/imgui.h>

#include "beat_clock.h"
#include "color.h"
#include "imgui_util.h"
#include "math_util.h"
#include "renderer.h"

void MechButton::ResetProps() {
    _p.key = imgui_util::Char(); 
    _p.offset = Vec3();
}

void MechButton::ResetState() {
    _s.keyPressed = false;
    _s.keyJustPressed = false;
    _s.keyJustReleased = false;
    _s.keyPushFactor = 0.f;
}

void MechButton::Save(serial::Ptree pt) const {
    _p.key.Save(pt, "key");
    serial::SaveInNewChildOf(pt, "offset", _p.offset);
}

void MechButton::Load(serial::Ptree pt) {
    ResetProps();
    _p.key.Load(pt, "key");
    serial::LoadFromChildOf(pt, "offset", _p.offset);
}

bool MechButton::ImGui() {
    bool changed = false;
    if (_p.key.ImGui("Key")) {
        changed = true;
    }
    if (imgui_util::InputVec3("Key offset", &_p.offset)) {
        changed = true;
    }
    return changed;
}

void MechButton::Init() {
    ResetState();
}

void MechButton::Update(GameManager& g, float const dt) {
    InputManager::Key k = InputManager::CharToKey(_p.key.c);
    _s.keyJustPressed = g._inputManager->IsKeyPressedThisFrame(k);
    _s.keyJustReleased = g._inputManager->IsKeyReleasedThisFrame(k);
    _s.keyPressed = g._inputManager->IsKeyPressed(k);
    float keyPushFactorTarget = _s.keyPressed ? 1.f : 0.f;
    _s.keyPushFactor += dt * 32.f * (keyPushFactorTarget - _s.keyPushFactor);

}

void MechButton::Draw(GameManager& g, Transform const& t) {
    Transform cubeCtrToBtm;
    cubeCtrToBtm.SetPos(Vec3(0.f, 0.5f, 0.f));
    Mat4 toBtmM = cubeCtrToBtm.Mat4NoScale();
   
    Vec4 keyNominalColor;
    float minV, maxV;
    if (!_s.keyPressed) {
        keyNominalColor.Set(1.0f, 1.f, 1.f, 1.f);
        minV = 1.f;
        maxV = 1.f;
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
    textBuf[0] = toupper(_p.key.c);
    size_t const textId = g._scene->DrawText3d(textBuf, 1, textTrans.Mat4Scale(), textColor, &textBbox); 
    Vec3 textOriginToCenter;
    textOriginToCenter._x = 0.5f * (textBbox.minX + textBbox.maxX);
    textOriginToCenter._y = 0.f;
    textOriginToCenter._z = 0.5f * (textBbox.minY + textBbox.maxY);
    Vec3 textExtents;
    textExtents._x = textBbox.maxX - textBbox.minX;
    textExtents._y = 0.f;
    textExtents._z = textBbox.maxY - textBbox.minY;

    float constexpr kButtonMaxHeight = 0.4f;
    float constexpr kButtonPushedHeight = 0.1f;
    float const btnHeight = math_util::Lerp(kButtonMaxHeight, kButtonPushedHeight, _s.keyPushFactor);

    Vec4 btnColor(193.f / 255.f, 108.f / 255.f, 0.f / 255.f, 1.f);
    Transform eToButtonT;
    Vec3 p = _p.offset;
    eToButtonT.SetPos(p);
    
    Vec3 btnScale = textExtents + Vec3(0.25f, 0.f, 0.25f);
    btnScale._y = btnHeight;
    eToButtonT.SetScale(btnScale);
    Mat4 btnRenderM = t.Mat4NoScale() * eToButtonT.Mat4Scale() * toBtmM;
    g._scene->DrawCube(btnRenderM, btnColor);
    
    Transform btnToText;
    Vec3 btnToTextP = -textOriginToCenter + Vec3(0.f, btnHeight + 0.001f, 0.f);
    btnToText.SetPos(btnToTextP);
    Mat4 textRenderM = t.Mat4NoScale() * eToButtonT.Mat4NoScale() * btnToText.Mat4NoScale();
    renderer::Glyph3dInstance& glyph3d = g._scene->GetText3d(textId);
    glyph3d._t = textRenderM;

}
