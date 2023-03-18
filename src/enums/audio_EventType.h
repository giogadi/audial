#pragma once


namespace audio {


enum class EventType : int {
    
    None,
    
    NoteOn,
    
    NoteOff,
    
    AllNotesOff,
    
    SynthParam,
    
    PlayPcm,
    
    StopPcm,
    
    SetGain,
    
    Count
};
extern char const* gEventTypeStrings[];
char const* EventTypeToString(EventType e);
EventType StringToEventType(char const* s);

bool EventTypeImGui(char const* label, EventType* v);


}
