#pragma once


namespace audio {


enum class EventType : int {
    
    None,
    
    NoteOn,
    
    NoteOff,
    
    SynthParam,
    
    PlayPcm,
    
    Count
};
char const* EventTypeToString(EventType e);
EventType StringToEventType(char const* s);


}
