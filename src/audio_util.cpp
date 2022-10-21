#include "audio_util.h"

void audio::Event::Save(serial::Ptree pt) const {
    std::string eventTypeStr = EventTypeToString(type);
    pt.PutString("event_type", eventTypeStr.c_str());
    pt.PutInt("channel", channel);
    pt.PutLong("tick_time", timeInTicks);    
    switch (type) {
        case EventType::SynthParam:
            pt.PutString("synth_param", SynthParamTypeToString(param));
            pt.PutDouble("value", newParamValue);
            break;
        case EventType::PlayPcm:
            pt.PutInt("sound_ix", pcmSoundIx);
            pt.PutFloat("velocity", velocity);
            pt.PutBool("loop", loop);
            break;
        case EventType::NoteOn:
            pt.PutInt("midi_note", midiNote);
            pt.PutInt("velocity", velocity);
            break;
        case EventType::NoteOff:
            pt.PutInt("midi_note", midiNote);
            break;
        case EventType::AllNotesOff:
            break;
        case EventType::None:
        case EventType::Count:
            break;
    }
}
void audio::Event::Load(serial::Ptree pt) {
    std::string eventTypeStr = pt.GetString("event_type");
    type = StringToEventType(eventTypeStr.c_str());
    channel = pt.GetInt("channel");
    timeInTicks = pt.GetLong("tick_time");
    switch (type) {
        case EventType::SynthParam: {
            std::string synthParamStr = pt.GetString("synth_param");
            param = StringToSynthParamType(synthParamStr.c_str());
            newParamValue = pt.GetDouble("value");
            break;
        }            
        case EventType::PlayPcm:
            if (!pt.TryGetInt("sound_ix", &pcmSoundIx)) {
                pcmSoundIx = pt.GetInt("midi_note");
            }
            pcmVelocity = pt.GetFloat("velocity");
            loop = false;
            pt.TryGetBool("loop", &loop);
            break;
        case EventType::NoteOn:
            midiNote = pt.GetInt("midi_note");
            velocity = pt.GetFloat("velocity");
            break;
        case EventType::NoteOff:
            midiNote = pt.GetInt("midi_note");
            break;
        case EventType::AllNotesOff:
            break;
        case EventType::None:
        case EventType::Count:
            break;
    }
}