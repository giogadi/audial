#include "events_on_hit.h"

#include <sstream>

#include "imgui/imgui.h"

#include "beat_clock.h"
#include "audio.h"
#include "rigid_body.h"
#include "audio_event_imgui.h"

bool EventsOnHitComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _t = e.FindComponentOfType<TransformComponent>();
    if (_t.expired()) {
        return false;
    }
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        return false;
    }
    _audio = g._audioContext;
    _beatClock = g._beatClock;
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        return false;
    }
    auto pThisComp = e.FindComponentOfType<EventsOnHitComponent>();    
    RigidBodyComponent& rb = *_rb.lock();
    rb.AddOnHitCallback(std::bind(&EventsOnHitComponent::OnHit, pThisComp, std::placeholders::_1));
    rb._layer = CollisionLayer::None;
    return true;
}

void EventsOnHitComponent::OnHit(
    std::weak_ptr<EventsOnHitComponent> thisComp, std::weak_ptr<RigidBodyComponent> other) {
    // Assumes neither object got destroyed. But at least we are able to handle this case later.
    thisComp.lock()->_wasHit = other.lock()->_layer == CollisionLayer::BodyAttack;
}

void EventsOnHitComponent::PlayEventsOnNextDenom(double denom) {
    double beatTime = _beatClock->GetBeatTime();
    double startTime = BeatClock::GetNextBeatDenomTime(beatTime, denom);
    unsigned long startTickTime = _beatClock->BeatTimeToTickTime(startTime);
    // std::cout << "TIME: " << startTickTime << std::endl;
    for (BeatTimeEvent const& event : _events) {
        audio::Event e = event._e;
        e.timeInTicks = _beatClock->BeatTimeToTickTime(event._beatTime) + startTickTime;
        _audio->AddEvent(e);
    }
}

void EventsOnHitComponent::Update(float dt) {
    if (!_wasHit) {
        return;
    }
    _wasHit = false;

    PlayEventsOnNextDenom(_denom);
}

bool EventsOnHitComponent::DrawImGui() {
    ImGui::InputScalar("Denom##", ImGuiDataType_Double, &_denom, /*step=*/nullptr, /*???*/nullptr, "%f");

    if (ImGui::Button("Add Event##")) {
        _events.emplace_back();
    }

    char headerName[128];
    for (int i = 0; i < _events.size(); ++i) {
        ImGui::PushID(i);
        sprintf(headerName, "%s##Header", audio::EventTypeToString(_events[i]._e.type));
        if (ImGui::CollapsingHeader(headerName)) {
            ImGui::InputScalar("Beat time##", ImGuiDataType_Float, &_events[i]._beatTime, /*step=*/nullptr, /*???*/nullptr, "%f");
            audio::EventDrawImGuiNoTime(_events[i]._e);
        }
        ImGui::PopID();
    }

    return false;
}

void EventsOnHitComponent::OnEditPick() {
    PlayEventsOnNextDenom(_denom);
}