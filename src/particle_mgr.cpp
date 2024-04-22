#include <particle_mgr.h>

#include <renderer.h>

namespace {
// returns true if swapped something else in
bool SwapRemove(Particle* particles, int* count, int removeIx) {
    if (removeIx + 1 < *count) {
        particles[removeIx] = particles[*count - 1];
        *count -= 1;
        return true;
    } else {
        *count -= 1;
        return false;
    }
}
}

Particle* ParticleMgr::SpawnParticle() {
    if (_particleCount >= MAX_PARTICLE_COUNT) {
        printf("Ran out of particles!\n");
        return nullptr;
    }
    return &_particles[_particleCount++];
}

void ParticleMgr::Update(float dt) {
    for (int ii = 0; ii < _particleCount; ++ii) {
        Particle* p = &_particles[ii];
        p->timeLeft -= dt;
        if (p->timeLeft <= 0.f) {
            if (SwapRemove(_particles, &_particleCount, ii)) {
                --ii;
            }
            continue;
        }
        p->t.Translate(p->v * dt);

        // Can we do this more efficiently?
        Vec3 axis = p->rotV;
        float rotSpeed = axis.Normalize();
        Quaternion qRot;
        qRot.SetFromAngleAxis(rotSpeed * dt, axis);

        Quaternion q = qRot * p->t.Quat();
        p->t.SetQuat(q);

        p->color._w += p->alphaV * dt;
    }
}

void ParticleMgr::Draw(GameManager& g) {
    BoundMeshPNU const* triMesh = g._scene->GetMesh("triangle");
    BoundMeshPNU const* cubeMesh = g._scene->GetMesh("cube"); 
    for (int ii = 0; ii < _particleCount; ++ii) {
        Particle* p = &_particles[ii];
        BoundMeshPNU const* mesh = nullptr;
        switch (p->shape) {
            case ParticleShape::Triangle: mesh = triMesh; break;
            case ParticleShape::Cube: mesh = cubeMesh; break;
        }
        renderer::ModelInstance& m = g._scene->DrawMesh(mesh, p->t.Mat4Scale(), p->color);
        m._castShadows = false;
        // m._sortTransparent = false;
    }
}
