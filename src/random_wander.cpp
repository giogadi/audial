#include "random_wander.h"

#include "imgui_util.h"

void RandomWander::Save(serial::Ptree pt) const {
    _bounds.Save(pt);
    pt.PutFloat("speed", _speed);
}

void RandomWander::Load(serial::Ptree pt) {
    _bounds.Load(pt);
    _speed = pt.GetFloat("speed");
}

bool RandomWander::ImGui() {
    imgui_util::InputVec3("bounds_min", &_bounds._min);
    imgui_util::InputVec3("bounds_max", &_bounds._max);
    ImGui::InputFloat("speed", &_speed);
    return true;
}

void RandomWander::Init() {
    _timeUntilNewPoint = -1.f;    
}

void RandomWander::Update(GameManager& g, float dt, ne::Entity* entity) {
    Vec3 entityPos = entity->_transform.Pos();
    
    // TODO curved paths?
    // TODO sync to the beat!!!
    if (_timeUntilNewPoint < 0.f && _speed > 0.00001f) {
        Vec3 randPoint;
        int constexpr kMaxTries = 10;
        bool success = false;
        for (int i = 0; i < kMaxTries; ++i) {
            randPoint = _bounds.SampleRandom();
            Vec3 entityToRand = randPoint - entityPos;
            float d2 = entityToRand.Length2();
            constexpr float kMinDist = 0.1f;
            if (d2 > kMinDist*kMinDist) {
                float d = sqrt(d2);
                _timeUntilNewPoint = d / _speed;
                _currentVel = entityToRand * (_speed / d);
                success = true;
                break;
            }
        }
        if (!success) {
            printf("RandomWander: failed to find a valid sample point\n");
        }
    }

    if (_timeUntilNewPoint >= 0.f) {
        Vec3 dp = _currentVel * dt;
        entityPos += dp;
        entity->_transform.SetPos(entityPos);
        _timeUntilNewPoint -= dt;
    }
}
