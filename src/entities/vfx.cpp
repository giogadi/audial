#include "vfx.h"

#include "math_util.h"

void VfxEntity::InitDerived(GameManager& g) {
    _timeSinceStart = -1.f;
}

void VfxEntity::OnHit(GameManager& g) {
    _timeSinceStart = 0.f;
}

void VfxEntity::Update(GameManager& g, float dt) {
    if (_timeSinceStart >= _p._cycleTimeSecs) {
        _transform.SetScale(_initTransform.Scale());
        _timeSinceStart = -1.f;
    } else if (_timeSinceStart >= 0.f) {
        float factor = _timeSinceStart / _p._cycleTimeSecs;
        factor = math_util::SmoothUpAndDown(factor);
        Vec3 pulseScale = _p._pulseScaleFactor * _initTransform.Scale();
        Vec3 s = math_util::Vec3Lerp(factor, _initTransform.Scale(), pulseScale);
        _transform.SetScale(s);
        _timeSinceStart += dt;
    }

    BaseEntity::Update(g, dt);
}
