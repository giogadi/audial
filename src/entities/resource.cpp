#include "resource.h"

#include "game_manager.h"
#include "renderer.h"
#include "math_util.h"
#include "particle_mgr.h"
#include "rng.h"

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
        constexpr float kMinYOffset = 0.f;

        if (_s.expandTimeElapsed < kExpandTime) {
            float expandFactor = math_util::InverseLerp(0.f, kExpandTime, _s.expandTimeElapsed);
            Vec3 scale = math_util::Vec3Lerp(expandFactor, kMinScale, kExpandScale);
            _s.renderT.SetScale(scale);

            Vec3 p = _transform.Pos() + Vec3(0.f, kMinYOffset, 0.f) * (1.f - expandFactor);
            _s.renderT.SetPos(p);
        } else if (_s.expandTimeElapsed < kExpandTime + kContractTime) {
            float factor = math_util::InverseLerp(kExpandTime, kExpandTime + kContractTime, _s.expandTimeElapsed);
            Vec3 scale = math_util::Vec3Lerp(factor, kExpandScale, _transform.Scale());
            _s.renderT.SetScale(scale);
        } else {
            _s.expandTimeElapsed = -1.f;
        }
    }

    if (_s.destroyTimeElapsed >= 0.f) {
        _s.destroyTimeElapsed += dt;

        constexpr Vec3 kMinScale(0.001f, 0.001f, 0.001f);
        const Vec3 kExpandScale = _transform.Scale() * 2.f;
        constexpr float kExpandTime = 0.0625f;
        constexpr float kContractTime = 0.0625f;
        constexpr float kMinYOffset = 0.f;

        if (_s.destroyTimeElapsed < kExpandTime) {
            float expandFactor = math_util::InverseLerp(0.f, kExpandTime, _s.destroyTimeElapsed);
            Vec3 scale = math_util::Vec3Lerp(expandFactor, _transform.Scale(), kExpandScale);
            _s.renderT.SetScale(scale);

        } else if (_s.destroyTimeElapsed < kExpandTime + kContractTime) {
            float factor = math_util::InverseLerp(kExpandTime, kExpandTime + kContractTime, _s.destroyTimeElapsed);
            Vec3 scale = math_util::Vec3Lerp(factor, kExpandScale, kMinScale);
            _s.renderT.SetScale(scale);
            Vec3 p = _transform.Pos() + Vec3(0.f, kMinYOffset, 0.f) * (factor);
            _s.renderT.SetPos(p);           
        } else {
            _s.destroyTimeElapsed = -1.f;
            g._neEntityManager->TagForDestroy(_id);
        }
    }    

}

void ResourceEntity::Draw(GameManager& g, float dt) {
    assert(!(_s.destroyed && _s.destroyTimeElapsed < 0.f));
    g._scene->DrawCube(_s.renderT.Mat4Scale(), _modelColor);
}

void ResourceEntity::StartConsume() {
    _s.destroyed = true;
    _s.destroyTimeElapsed = 0.f;
    _wpFollower.Stop();
}

void ResourceEntity::StartDestroy(GameManager& g) {
    g._neEntityManager->TagForDestroy(_id);
    _wpFollower.Stop();

    int constexpr kNumParticles = 20;
    float constexpr kParticleSpeed = 3.f;
    float constexpr kParticleTime = 0.4f;
    Quaternion q;
    float alphaV = -1.f / kParticleTime;
    for (int ii = 0; ii < kNumParticles; ++ii) {
        float x = rng::GetFloatGlobal(-kPi, kPi);
        float y = rng::GetFloatGlobal(-kPi, kPi);
        float z = rng::GetFloatGlobal(-kPi, kPi);
        Vec3 r(x, y, z);
        q.SetFromEulerAngles(r);

        r.Normalize();

        Particle* p = g._particleMgr->SpawnParticle();
        p->shape = ParticleShape::Cube;
        Vec3 pos = _transform.Pos();
        pos += (r * 0.5f);
        p->t.SetPos(pos);
        p->t.SetQuat(q);
        p->t.SetScale(Vec3(0.2f, 0.2f, 0.2f));

        p->v = r * kParticleSpeed;
        p->rotV = r * 3.f;

        p->color = _modelColor;
        p->timeLeft = 2.f;

        p->alphaV = alphaV;
    }
}
