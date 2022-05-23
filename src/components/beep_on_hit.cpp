#include "beep_on_hit.h"

#include <sstream>

#include "imgui/imgui.h"

#include "beat_clock.h"
#include "audio.h"
#include "rigid_body.h"

bool BeepOnHitComponent::ConnectComponents(Entity& e, GameManager& g) {
    bool success = true;
    _t = e.FindComponentOfType<TransformComponent>();
    if (_t.expired()) {
        success = false;
    }
    _audio = g._audioContext;
    _beatClock = g._beatClock;
    std::weak_ptr<RigidBodyComponent> pRb = e.FindComponentOfType<RigidBodyComponent>();
    if (pRb.expired()) {
        success = false;
    } else {
        std::weak_ptr<BeepOnHitComponent> pBeep = e.FindComponentOfType<BeepOnHitComponent>();
        RigidBodyComponent& rb = *pRb.lock();
        rb.SetOnHitCallback(std::bind(&BeepOnHitComponent::OnHit, pBeep, std::placeholders::_1));
    }
    return success;
}

void BeepOnHitComponent::OnHit(
    std::weak_ptr<BeepOnHitComponent> beepComp, std::weak_ptr<RigidBodyComponent> other) {
    // Assumes neither object got destroyed. But at least we are able to handle this case later.
    beepComp.lock()->_wasHit = other.lock()->_layer == CollisionLayer::BodyAttack;
}

void BeepOnHitComponent::Update(float dt) {
    if (!_wasHit) {
        return;
    }
    _wasHit = false;

    double beatTime = _beatClock->GetBeatTime();
    double denom = 0.25;
    // double denom = 1.0;
    double noteTime = BeatClock::GetNextBeatDenomTime(beatTime, denom);
    double noteOffTime = noteTime + 0.5 * denom;
    if (noteOffTime > _lastScheduledEvent) {
        _lastScheduledEvent = noteOffTime;

        audio::Event e;
        e.channel = _synthChannel;
        for (int note : _midiNotes) {
            if (note < 0) {
                continue;
            }
            e.midiNote = note;
            e.type = audio::EventType::NoteOn;
            e.timeInTicks = _beatClock->BeatTimeToTickTime(noteTime);
            _audio->AddEvent(e);
            e.type = audio::EventType::NoteOff;
            e.timeInTicks = _beatClock->BeatTimeToTickTime(noteOffTime);
            _audio->AddEvent(e);
        }
    }
}

void BeepOnHitComponent::DrawImGui() {
    ImGui::InputScalar("Channel", ImGuiDataType_S32, &_synthChannel, /*step=*/nullptr, /*???*/NULL, "%d");
    char label[] = "Note XXX";
    for (int i = 0; i < _midiNotes.size(); ++i) {        
        sprintf(label, "Note %d", i);
        ImGui::InputScalar(
            label, ImGuiDataType_S32, &(_midiNotes[i]), /*step=*/nullptr, /*???*/NULL, "%d");
    }
}