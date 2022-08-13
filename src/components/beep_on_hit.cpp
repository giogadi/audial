#include "beep_on_hit.h"

#include "imgui/imgui.h"

#include "beat_clock.h"
#include "audio.h"
#include "rigid_body.h"
#include "components/transform.h"
#include "entity.h"
#include "game_manager.h"

bool BeepOnHitComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
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
    auto pThisComp = e.FindComponentOfType<BeepOnHitComponent>();
    RigidBodyComponent& rb = *_rb.lock();
    rb.AddOnHitCallback(std::bind(&BeepOnHitComponent::OnHit, pThisComp, std::placeholders::_1));
    rb._layer = CollisionLayer::None;
    return true;
}

void BeepOnHitComponent::OnHit(
    std::weak_ptr<BeepOnHitComponent> beepComp, std::weak_ptr<RigidBodyComponent> other) {
    // Assumes neither object got destroyed. But at least we are able to handle this case later.
    beepComp.lock()->_wasHit = other.lock()->_layer == CollisionLayer::BodyAttack;
}

void BeepOnHitComponent::PlayNotesOnNextDenom(double denom) {
    double beatTime = _beatClock->GetBeatTime();
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

void BeepOnHitComponent::Update(float dt) {
    if (!_wasHit) {
        return;
    }
    _wasHit = false;

    PlayNotesOnNextDenom(/*denom=*/0.25);
}

bool BeepOnHitComponent::DrawImGui() {
    ImGui::InputScalar("Channel", ImGuiDataType_S32, &_synthChannel, /*step=*/nullptr, /*???*/NULL, "%d");
    char label[] = "Note XXX";
    for (int i = 0; i < _midiNotes.size(); ++i) {
        sprintf(label, "Note %d", i);
        ImGui::InputScalar(
            label, ImGuiDataType_S32, &(_midiNotes[i]), /*step=*/nullptr, /*???*/NULL, "%d");
    }
    return false;
}

void BeepOnHitComponent::OnEditPick() {
    PlayNotesOnNextDenom(/*denom=*/0.25);
}