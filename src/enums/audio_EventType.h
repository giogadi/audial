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
    
    Count
};
extern char const* gEventTypeStrings[];
char const* EventTypeToString(EventType e);
EventType StringToEventType(char const* s);


}
