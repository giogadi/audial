#include "audio_event_imgui.h"

#include "imgui/imgui.h"

#include "audio_util.h"
#include "beat_clock.h"
#include "sound_bank.h"

namespace audio {

void EventDrawImGuiNoTime(Event& event, SoundBank const& soundBank) {
    int selectedEventTypeIx = static_cast<int>(event.type);
    bool changed = ImGui::Combo(
        "EventType##", &selectedEventTypeIx, gEventTypeStrings, static_cast<int>(EventType::Count));
    if (changed) {
        // When type changes, re-initialize the event from scratch.
        event = audio::Event();
        event.type = static_cast<EventType>(selectedEventTypeIx);
    }
    switch (event.type) {
        case EventType::NoteOn:
        case EventType::NoteOff: {
            ImGui::InputScalar(
                "Channel##", ImGuiDataType_S32, &event.channel, /*step=*/nullptr, /*???*/nullptr, "%d");
            ImGui::InputScalar(
                "Note##", ImGuiDataType_S32, &event.midiNote, /*step=*/nullptr, /*???*/nullptr, "%d");
            ImGui::InputScalar("Vel##", ImGuiDataType_Float, &event.velocity);
            break;
        }
        case audio::EventType::AllNotesOff: {
            ImGui::InputScalar(
                "Channel##", ImGuiDataType_S32, &event.channel, /*step=*/nullptr, /*???*/nullptr, "%d");        
            break;
        }
        case audio::EventType::PlayPcm: {
            ImGui::Combo("Sound##", &event.pcmSoundIx, soundBank._soundNames.data(), soundBank._soundNames.size());
            ImGui::InputScalar("Vel##", ImGuiDataType_Float, &event.pcmVelocity);
            ImGui::Checkbox("Loop##", &event.loop);
            break;
        }
        case audio::EventType::StopPcm: {
            ImGui::Combo("Sound##", &event.pcmSoundIx, soundBank._soundNames.data(), soundBank._soundNames.size());
            break;
        }
        case audio::EventType::SetGain: {
            ImGui::InputScalar("Gain##", ImGuiDataType_Float, &event.newGain);
            break;
        }
        case audio::EventType::None:
        case audio::EventType::SynthParam:        
        case audio::EventType::Count:
            ImGui::Text("UNSUPPORTED");
            break;
    }
}

}  // namespace
