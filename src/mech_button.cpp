#include "mech_button.h"

#include <imgui/imgui.h>

#include "beat_clock.h"
#include "color.h"
#include "imgui_util.h"
#include "math_util.h"
#include "renderer.h"

void MechButton::Save(serial::Ptree pt) const {
    key.Save(pt, "key");
    serial::SaveInNewChildOf(pt, "offset", offset);
}

void MechButton::Load(serial::Ptree pt) {
    key.Load(pt, "key");
    serial::LoadFromChildOf(pt, "offset", offset);
}

bool MechButton::ImGui() {
    bool changed = false;
    if (key.ImGui("Key")) {
        changed = true;
    }
    if (imgui_util::InputVec3("Key offset", &offset)) {
        changed = true;
    }
    return changed;
}

void MechButton::Draw(GameManager& g, Transform const& t, float const pushFactor, bool const keyPressed) {
    Transform cubeCtrToBtm;
    cubeCtrToBtm.SetPos(Vec3(0.f, 0.5f, 0.f));
    Mat4 toBtmM = cubeCtrToBtm.Mat4NoScale();
   
    Vec4 keyNominalColor;
    float minV, maxV;
    if (!keyPressed) {
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
    textBuf[0] = toupper(key.c);
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
    float const btnHeight = math_util::Lerp(kButtonMaxHeight, kButtonPushedHeight, pushFactor);

    Vec4 btnColor(188.f / 255.f, 132.f / 255.f, 0.f / 255.f, 1.f);
    Transform eToButtonT;
    Vec3 p = offset;
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
