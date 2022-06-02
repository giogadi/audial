#include "events_on_hit.h"

#include <sstream>

#include "imgui/imgui.h"

#include "beat_clock.h"
#include "audio.h"
#include "rigid_body.h"

bool EventsOnHitComponent::ConnectComponents(Entity& e, GameManager& g) {
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
    for (audio::Event event : _events) {
        event.timeInTicks += startTickTime;
        _audio->AddEvent(event);
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
    // Maybe for now, events are either "note on" or "note off" and each come with note+time.
    int eventTypeIx = -1;
    ImGui::ListBox(
        "Events##", &eventTypeIx, audio::gEventTypeStrings,
        /*numItems=*/static_cast<int>(audio::EventType::Count));
    // if (ImGui::Button("Add Event")) {
    //     audio::Event event;
    //     event.type = audio::EventType::NoteOn;
    //     event.channel = 0;
    //     event.timeInTicks = 0l;
    //     _events.push_back(std::move(event));
    // }

    // for (int i = 0; i < _events.size(); ++i) {

    // }

    // ImGui::InputScalar("Channel", ImGuiDataType_S32, &_synthChannel, /*step=*/nullptr, /*???*/NULL, "%d");
    // char label[] = "Note XXX";
    // for (int i = 0; i < _midiNotes.size(); ++i) {        
    //     sprintf(label, "Note %d", i);
    //     ImGui::InputScalar(
    //         label, ImGuiDataType_S32, &(_midiNotes[i]), /*step=*/nullptr, /*???*/NULL, "%d");
    // }
    // return false;
    return false;
}

void EventsOnHitComponent::OnEditPick() {
    PlayEventsOnNextDenom(_denom);
}