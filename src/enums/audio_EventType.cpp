#include "src/enums/audio_EventType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"


namespace audio {


namespace {

std::unordered_map<std::string, EventType> const gStringToEventType = {
    
    { "None", EventType::None },
    
    { "NoteOn", EventType::NoteOn },
    
    { "NoteOff", EventType::NoteOff },
    
    { "AllNotesOff", EventType::AllNotesOff },
    
    { "SynthParam", EventType::SynthParam },
    
    { "PlayPcm", EventType::PlayPcm },
    
    { "StopPcm", EventType::StopPcm },
    
    { "SetGain", EventType::SetGain }
    
};

} // end namespace

char const* gEventTypeStrings[] = {
	
    "None",
    
    "NoteOn",
    
    "NoteOff",
    
    "AllNotesOff",
    
    "SynthParam",
    
    "PlayPcm",
    
    "StopPcm",
    
    "SetGain"
    
};

char const* EventTypeToString(EventType e) {
	return gEventTypeStrings[static_cast<int>(e)];
}

EventType StringToEventType(char const* s) {
    auto iter = gStringToEventType.find(s);
    if (iter != gStringToEventType.end()) {
    	return gStringToEventType.at(s);
    }
    printf("ERROR StringToEventType: unrecognized value \"%s\"\n", s);
    return static_cast<EventType>(0);
}

bool EventTypeImGui(char const* label, EventType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gEventTypeStrings, static_cast<int>(EventType::Count));
    if (changed) {
        *v = static_cast<EventType>(selectedIx);
    }
    return changed;
}


}

char const* EnumToString(audio::EventType e) {
     return audio::EventTypeToString(e);
}

void StringToEnum(char const* s, audio::EventType& e) {
     e = audio::StringToEventType(s);
}