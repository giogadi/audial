#include "src/enums/audio_EventType.h"

#include <unordered_map>
#include <string>


namespace audio {


namespace {

std::unordered_map<std::string, EventType> const gStringToEventType = {
    
    { "None", EventType::None },
    
    { "NoteOn", EventType::NoteOn },
    
    { "NoteOff", EventType::NoteOff },
    
    { "AllNotesOff", EventType::AllNotesOff },
    
    { "SynthParam", EventType::SynthParam },
    
    { "PlayPcm", EventType::PlayPcm },
    
    { "StopPcm", EventType::StopPcm }
    
};

} // end namespace

char const* gEventTypeStrings[] = {
	
    "None",
    
    "NoteOn",
    
    "NoteOff",
    
    "AllNotesOff",
    
    "SynthParam",
    
    "PlayPcm",
    
    "StopPcm"
    
};

char const* EventTypeToString(EventType e) {
	return gEventTypeStrings[static_cast<int>(e)];
}

EventType StringToEventType(char const* s) {
	return gStringToEventType.at(s);
}


}
