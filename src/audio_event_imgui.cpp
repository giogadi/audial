#include "audio_event_imgui.h"

#include "imgui/imgui.h"

#include "audio_util.h"
#include "beat_clock.h"

namespace audio {

void EventDrawImGuiNoTime(Event& event) {
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
            ImGui::InputScalar("Sound##", ImGuiDataType_S32, &event.midiNote, /*step=*/nullptr, /*???*/nullptr, "%d");
            ImGui::InputScalar("Vel##", ImGuiDataType_Float, &event.velocity);
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