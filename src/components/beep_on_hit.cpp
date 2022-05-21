#include "beep_on_hit.h"

#include <sstream>

#include "imgui/imgui.h"

#include "beat_clock.h"
#include "audio.h"
#include "rigid_body.h"

void BeepOnHitComponent::ConnectComponents(Entity& e, GameManager& g) {    
    _t = e.DebugFindComponentOfType<TransformComponent>();
    _audio = g._audioContext;
    _beatClock = g._beatClock;
    RigidBodyComponent* rb = e.DebugFindComponentOfType<RigidBodyComponent>();
    rb->SetOnHitCallback(std::bind(&BeepOnHitComponent::OnHit, this, std::placeholders::_1));
}

void BeepOnHitComponent::OnHit(RigidBodyComponent* other) {
    _wasHit = other->_layer == CollisionLayer::BodyAttack;
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