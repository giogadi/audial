#include "entities/particle_emitter.h"

#include <imgui/imgui.h>

#include "game_manager.h"
#include "rng.h"
#include "particle_mgr.h"
#include "constants.h"
#include "math_util.h"

void ParticleEmitterEntity::SaveDerived(serial::Ptree pt) const {
}

void ParticleEmitterEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
}

ne::BaseEntity::ImGuiResult ParticleEmitterEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}

ne::BaseEntity::ImGuiResult ParticleEmitterEntity::MultiImGui(GameManager& g, BaseEntity** entities, size_t entityCount) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}


void ParticleEmitterEntity::InitDerived(GameManager& g) {
    _s = State();

}

namespace {
#if 0
    void EmitParticlesSpiral(GameManager& g, Vec3 const& entityPrevPos, Vec3 const& entityPos, float timeLeft, float totalTime, Vec3 const& entityDir, float const dt) {
        float particleSpeed = 0.f;
        int constexpr kNumSteps = 8;
        static float stepTheta = 0.f;
        float constexpr kMaxAngleVel = 60.f * k2Pi;
        float constexpr kMinAngleVel = kMaxAngleVel;
        Vec3 blastDir = -entityDir;
        Vec4 color0(0., 1.f, 1.f, 1.f);
        Vec4 color1(0.8f, 0.8f, 0.8f, 1.f);
        for (int ii = 0; ii < kNumSteps; ++ii) {
            float const factor = (float)ii / (float)kNumSteps;
            Vec3 particlePos = math_util::Vec3Lerp(factor, entityPrevPos, entityPos);

            float motionFactor = (timeLeft - factor * dt) / totalTime;

            float angleVel = math_util::Lerp(kMinAngleVel, kMaxAngleVel, motionFactor);
            stepTheta += angleVel * dt * (1.f / kNumSteps);
            int constexpr kNumParticles = 1;

            Vec4 color = math_util::Vec4Lerp(color0, color1, motionFactor);
            for (int jj = 0; jj < kNumParticles; ++jj) {
                float theta = stepTheta + jj * (k2Pi / kNumParticles);
                Vec3 localZ = blastDir;
                Vec3 localY(0.f, 1.f, 0.f);
                Vec3 localX = Vec3::Cross(localY, localZ);
                Vec3 particleOffset = localX * cos(theta) + localY * sin(theta);

                Particle* particle = g._particleMgr->SpawnParticle();
                particle->t.SetPos(particlePos + particleOffset * 0.2f);
                {
                    Mat3 rot;
                    rot.SetCol(0, localX);
                    rot.SetCol(1, localY);
                    rot.SetCol(2, localZ);
                    Quaternion q;
                    q.SetFromRotMat(rot);
                    particle->t.SetQuat(q);
                }
                particle->rotV = localZ * 5.f;
                float constexpr scale = 0.1f;
                particle->t.SetScale(Vec3(scale, scale, scale));
                particle->a = Vec3(0.f, 0.f, 4.f);
                particle->a += (particleOffset * 1.f * motionFactor);
                particle->a += (motionFactor * 2.f - 1.f) * blastDir * 2.f;
                //particle->a = (motionFactor * 2.f - 1.f) * blastDir * 1.f;
                particle->v = (particleOffset * particleSpeed) + particle->a * (1.f - factor) * dt;

                particle->alphaV = -1.f - 1.f * (motionFactor);
                color._w = 1.f + particle->alphaV * (1.f - factor) * dt;
                particle->color = color;

                particle->timeLeft = 2.f + factor * dt;
                particle->shape = ParticleShape::Cube;
            }
        }
    }
#endif
    // Shoot down +z direction
    void GenerateGustParticles(GameManager& g, ParticleEmitterEntity& emitter) {
        int constexpr kNumParticles = 3;
        Mat4 emitterTrans = emitter._transform.Mat4Scale();
        emitterTrans.ScaleUniform(0.5f);
        float constexpr kSize = 0.035f;
        Vec3 dir = emitter._transform.GetZAxis();
        float constexpr kInitSpeed = 30.f;
        for (int ii = 0; ii < kNumParticles; ++ii) {
            float x = rng::GetFloatGlobal(-1.f, 1.f);
            float y = rng::GetFloatGlobal(-1.f, 1.f);
            Vec4 localPos(x, y, 0.f, 1.f);
            Vec4 worldPos = emitterTrans * localPos;
            worldPos /= worldPos._w;  //?

            Particle* p = g._particleMgr->SpawnParticle();
            p->t.SetScale(Vec3(kSize, kSize, kSize));
            p->t.SetPos(worldPos.GetXYZ());
            p->extraScaleFromSpeed.Set(0.f, 0.f, 15.f / kInitSpeed);
            float noisySpeed = rng::GetFloatGlobal(kInitSpeed * 0.7f, kInitSpeed);
            p->v = dir * noisySpeed;
            //Vec3 noiseLocalX = emitter._transform.GetXAxis() * rng::GetFloatGlobal(-1.f, 1.f);
            //p->v += noiseLocalX;
            p->v += emitter._transform.GetXAxis() * x;
            {
                Vec3 velZ = p->v.GetNormalized();
                Vec3 velX = Vec3::Cross(Vec3(0.f, 1.f, 0.f), velZ);
                Mat3 rot;
                rot.SetCol(0, velX);
                rot.SetCol(1, Vec3(0.f, 1.f, 0.f));
                rot.SetCol(2, velZ);
                Quaternion q;
                q.SetFromRotMat(rot);
                p->t.SetQuat(q);
            }

            //p->minSpeed = rng::GetFloatGlobal(0.5f, 2.f);
            p->minSpeed = rng::GetFloatGlobal(-0.5f, 0.5f);
            p->minSpeedDir = dir;
            p->a = -dir * rng::GetFloatGlobal(70.f, 80.f);
            p->timeLeft = 2.f;
            p->alphaV = -0.75f;
            //p->color.Set(1.f, 1.f, 1.f, 1.f);
            p->color = emitter._modelColor;
            p->shape = ParticleShape::Cube;
        }
    }
}

void ParticleEmitterEntity::UpdateDerived(GameManager& g, float dt) {
    GenerateGustParticles(g, *this);
    ++_s._numEmitted;

    if (_s._numEmitted > 10) {
        g._neEntityManager->TagForDeactivate(_id);
    }    
}

void ParticleEmitterEntity::Draw(GameManager& g, float dt) {
    BaseEntity::Draw(g, dt);
}

void ParticleEmitterEntity::OnEditPick(GameManager& g) {
    //GenerateGustParticles(g, *this);
}
