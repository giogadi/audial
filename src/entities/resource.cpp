#include "resource.h"

#include "game_manager.h"
#include "renderer.h"
#include "math_util.h"

void ResourceEntity::SaveDerived(serial::Ptree pt) const {
}

void ResourceEntity::LoadDerived(serial::Ptree pt) {
}

void ResourceEntity::InitDerived(GameManager& g) {
    _s = State();
}

ne::Entity::ImGuiResult ResourceEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}

void ResourceEntity::UpdateDerived(GameManager& g, float dt) {
    _s.renderT = _transform;
    
    if (g._editMode) {
        return;
    }

    Vec3 p = _transform.Pos();
    Vec3 dp = _s.v * dt;
    p += dp;
    _transform.SetPos(p);

    if (_s.expandTimeElapsed >= 0.f) {
        _s.expandTimeElapsed += dt;

        constexpr Vec3 kMinScale(0.001f, 0.001f, 0.001f);
        const Vec3 kExpandScale = _transform.Scale() * 2.f;
        constexpr float kExpandTime = 0.0625f;
        constexpr float kContractTime = 0.0625f;

        if (_s.expandTimeElapsed < kExpandTime) {
            float expandFactor = math_util::InverseLerp(0.f, kExpandTime, _s.expandTimeElapsed);
            Vec3 scale = math_util::Vec3Lerp(expandFactor, kMinScale, kExpandScale);
            _s.renderT.SetScale(scale);
        } else if (_s.expandTimeElapsed < kExpandTime + kContractTime) {
            float factor = math_util::InverseLerp(kExpandTime, kExpandTime + kContractTime, _s.expandTimeElapsed);
            Vec3 scale = math_util::Vec3Lerp(factor, kExpandScale, _transform.Scale());
            _s.renderT.SetScale(scale);
        } else {
            _s.expandTimeElapsed = -1.f;
        }
    }
}

void ResourceEntity::Draw(GameManager& g, float dt) {
    g._scene->DrawCube(_s.renderT.Mat4Scale(), _modelColor);
}
