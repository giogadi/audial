#pragma once

#include <transform.h>
#include <game_manager.h>

#define MAX_PARTICLE_COUNT 1024

enum class ParticleShape {
    Triangle, Cube
};
struct Particle {
    Transform t;
    Vec3 v;
    Vec3 rotV;
    Vec4 color;
    float alphaV = 0.f;
    float timeLeft = 0.f;
    ParticleShape shape = ParticleShape::Triangle;
};
struct ParticleMgr {
    Particle* SpawnParticle();
    void Update(float dt);
    void Draw(GameManager& g);
    
    Particle _particles[MAX_PARTICLE_COUNT];
    int _particleCount = 0;
};
