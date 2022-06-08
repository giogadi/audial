#pragma once

#include <array>

#include "component.h"
#include "beat_time_event.h"

class BeatClock;
namespace audio {
    class Context;
}
class RigidBodyComponent;

class EventsOnHitComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::EventsOnHit; }
    EventsOnHitComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    static void OnHit(
        std::weak_ptr<EventsOnHitComponent> beepComp, std::weak_ptr<RigidBodyComponent> other);

    virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    virtual void OnEditPick() override;

    std::weak_ptr<TransformComponent> _t;
    std::weak_ptr<RigidBodyComponent> _rb;
    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    bool _wasHit = false;
    double _denom = 0.25;
    std::vector<BeatTimeEvent> _events;

private:
    void PlayEventsOnNextDenom(double denom);
};