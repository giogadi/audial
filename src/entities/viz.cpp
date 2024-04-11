#include "viz.h"

#include "game_manager.h"
#include "renderer.h"
#include "math_util.h"

extern double* gDftLogAvgs;
extern double* gDftLogAvgsSmooth;
extern double* gDftLogAvgsScaled;
extern int gDftLogAvgsCount;

void VizEntity::SaveDerived(serial::Ptree pt) const {
}

void VizEntity::LoadDerived(serial::Ptree pt) {
}

void VizEntity::InitDerived(GameManager& g) {
    _s = State();
}

ne::Entity::ImGuiResult VizEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}

void VizEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }

    Vec3 p = _transform.Pos();
    Vec3 dp = _s.v * dt;
    p += dp;
    _transform.SetPos(p);
}

void VizEntity::Draw(GameManager& g, float dt) {
    for (int ii = 0; ii < gDftLogAvgsCount; ++ii) {
        float s = gDftLogAvgsScaled[ii];

        // float s = gDftLogAvgsSmooth[ii];
        // float s = gDftLogAvgs[ii];
        // s = math_util::InverseLerp(0.f, 0.1f, s);
        s = math_util::Lerp(0.1f, 4.f, s);

        Transform offset;
        offset.SetPos(Vec3(0.f, 0.5f, 0.f));

        Transform scale;
        scale.SetScale(Vec3(1.f, s, 1.f));

        Mat4 offsetMat = offset.Mat4Scale();
        Mat4 scaleMat = scale.Mat4Scale();
        
        Transform binTrans = _transform;
        binTrans.Translate(Vec3(ii * 2.f, 0.f, 0.f));
        Mat4 binTransMat = binTrans.Mat4Scale();

        Mat4 finalMat = binTransMat * scaleMat * offsetMat;
        g._scene->DrawCube(finalMat, _modelColor);

    }
}
