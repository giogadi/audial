#include "recorder_balloon.h"

#include "components/events_on_hit.h"
#include "components/model_instance.h"
#include "entity.h"

bool RecorderBalloonComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _model = e.FindComponentOfType<ModelComponent>();
    if (_model.expired()) {
        return false;
    }

    auto const& onHitComp = e.FindComponentOfType<EventsOnHitComponent>().lock();
    if (onHitComp == nullptr) {
        return false;
    }
    onHitComp->AddOnHitCallback(
        std::bind(&RecorderBalloonComponent::OnHit, this, std::placeholders::_1));

    return true;
}

void RecorderBalloonComponent::OnHit(EntityId other) {
    _heat = std::min(1.f, _heat + 0.2f);
}

void RecorderBalloonComponent::Update(float dt) {
    float constexpr kHeatLossPerSec = 0.5f;

    _heat = std::max(0.f, _heat - kHeatLossPerSec * dt);

    Vec4 color(1.f, 1.f - _heat, 1.f - _heat, 1.f);
    auto const& model = _model.lock();
    assert(model != nullptr);
    model->_color = color;
}