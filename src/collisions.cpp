#include "collisions.h"

#include "components/transform.h"

namespace {
    bool gCollisionMatrix [static_cast<int>(CollisionLayer::Count)][static_cast<int>(CollisionLayer::Count)] = {
        { 0, 0, 0 }, // None
        { 0, 1, 1 }, // Solid
        { 0, 1, 1 }, // BodyAttack
    };
}

void CollisionManager::AddBody(std::weak_ptr<RigidBodyComponent> body) {
    _bodies.push_back(body);
}

namespace {
    bool aabbOverlap(Aabb const& a, Aabb const& b) {
        return !(a._min._x > b._max._x || a._max._x < b._min._x ||
                 a._min._y > b._max._y || a._max._y < b._min._y ||
                 a._min._z > b._max._z || a._max._z < b._min._z);
    }

    Aabb translateAabb(Aabb const& a, Vec3 const& translate) {
        Aabb newAabb = a;
        newAabb._min += translate;
        newAabb._max += translate;
        return newAabb;
    }

    // ONLY WORKS IF THEY'RE OVERLAPPING. This isn't a true penetration vec: it
    // just returns a push in one of the 3 axes that results in no more overlap. Result vector pushes a away from b.
    Vec3 GetPenetrationVec(Aabb const& a, Aabb const& b, float padding) {
        // find minimum penetration axis. ASSUME OVERLAP!!!
        Vec3 diff1 = a._min - b._max;
        Vec3 diff2 = a._max - b._min;
        float minDepth = std::numeric_limits<float>::max();
        int minDepthIdx = -1;
        bool minDepth1 = false;
        for (int i = 0; i < 3; ++i) {
            float d1 = std::abs(diff1[i]);
            if (d1 < minDepth) {
                minDepth = d1;
                minDepthIdx = i;
                minDepth1 = true;
            }

            float d2 = std::abs(diff2[i]);
            if (d2 < minDepth) {
                minDepth = d2;
                minDepthIdx = i;
                minDepth1 = false;
            }
        }

        Vec3 pushVec(0.f, 0.f, 0.f);
        if (minDepth1) {
            // This means we need to scooch along the POSITIVE minimum axis.
            pushVec[minDepthIdx] += minDepth + padding;
        } else {
            // This means we need to scooch along the NEGATIVE minimum axis.
            pushVec[minDepthIdx] -= minDepth + padding;
        }
        return pushVec;
    }
} // namespace

// For now, we're not gonna do anything clever about n-body collisions.
void CollisionManager::Update(float dt) {
    _bodies.erase(std::remove_if(_bodies.begin(), _bodies.end(),
        [](std::weak_ptr<RigidBodyComponent const> const& p) {
            return p.expired();
        }), _bodies.end());

    std::vector<Vec3> newPositions(_bodies.size());
    std::vector<Aabb> newAabbs(_bodies.size());
    // Stores pointer of other body hit. nullptr if no hit
    std::vector<bool> hits(_bodies.size(), false);
    std::vector<std::weak_ptr<RigidBodyComponent>> hitOthers(_bodies.size());
    for (int i = 0; i < _bodies.size(); ++i) {
        RigidBodyComponent& rb = *_bodies[i].lock();
        TransformComponent const& rbTrans = *rb._transform.lock();
        Vec3 const rbTranslateLocal = dt * rb._velocity;
        Vec3 newPosLocal = rbTrans.GetLocalPos() + rbTranslateLocal;
        Vec3 newPosWorld = rbTrans.TransformLocalToWorld(newPosLocal);
        Aabb newAabb = translateAabb(rb._localAabb, newPosWorld);
        newPositions[i] = newPosWorld;
        newAabbs[i] = newAabb;
    }

    for (int i = 0; i < _bodies.size(); ++i) {
        RigidBodyComponent& rb1 = *_bodies[i].lock();
        for (int j = (i+1); j < _bodies.size(); ++j) {
            RigidBodyComponent& rb2 = *_bodies[j].lock();
            if (!gCollisionMatrix[static_cast<int>(rb1._layer)][static_cast<int>(rb2._layer)]) {
                continue;
            }
            if (!aabbOverlap(newAabbs[i], newAabbs[j])) {
                continue;
            }
            hits[i] = hits[j] = true;
            hitOthers[i] = _bodies[j];
            hitOthers[j] = _bodies[i];

            // Our shitty collision response strategy: we generate an
            // axis-aligned vector of minimum motion to push rb1 out of rb2
            // (making silly assumption that rb2 doesn't move.) Then, if rb1 is
            // non-static we translate it by that vector. If rb2 is non-static,
            // we translate it by -vector. This is not a realistic interaction.
            // Also, we require at least one to be static or else we'll never
            // escape collision.
            Vec3 pushVec = GetPenetrationVec(newAabbs[i], newAabbs[j], /*padding=*/0.001f);
            assert(!rb1._static || !rb2._static);
            if (!rb1._static) {
                newPositions[i] += pushVec;
            }
            if (!rb2._static) {
                newPositions[j] -= pushVec;
            }

            // Simpler version of collision response that just restores the previous positions.
            // newPositions[i] = rb1._transform->GetPos();
            // newPositions[j] = rb2._transform->GetPos();
        }
    }

    for (int i = 0; i < _bodies.size(); ++i) {
        RigidBodyComponent& rb = *_bodies[i].lock();
        rb._transform.lock()->SetWorldPos(newPositions[i]);
        if (hits[i]) {
            rb.InvokeCallbacks(hitOthers[i]);
        }
    }
}