#include "flow_pickup.h"

#include "beat_clock.h"
#include "constants.h"

void FlowPickupEntity::Update(GameManager& g, float dt) {
    if (!g._editMode) {
        // Rotate about y-axis. half rotation every beat
        double beatTime = g._beatClock->GetBeatTimeFromEpoch();
        float angle = (float) beatTime * kPi;
        Quaternion q;
        q.SetFromAngleAxis(angle, Vec3(1.f, 1.f, 1.f).GetNormalized());
        _transform.SetQuat(q);
    }
    
    BaseEntity::Update(g, dt);
}
