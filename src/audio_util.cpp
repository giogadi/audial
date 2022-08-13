#include "audio_util.h"

void audio::Event::Save(serial::Ptree pt) const {
    std::string eventTypeStr = EventTypeToString(type);
    pt.PutString("event_type", eventTypeStr.c_str());
    pt.PutInt("channel", channel);
    pt.PutLong("tick_time", timeInTicks);
    if (type == EventType::SynthParam) {
        pt.PutString("synth_param", SynthParamTypeToString(param));
        pt.PutDouble("value", newParamValue);
    } else {
        pt.PutInt("midi_note", midiNote);
        pt.PutFloat("velocity", velocity);
    }
}
void audio::Event::Load(serial::Ptree pt) {
    std::string eventTypeStr = pt.GetString("event_type");
    type = StringToEventType(eventTypeStr.c_str());
    channel = pt.GetInt("channel");
    timeInTicks = pt.GetLong("tick_time");
    if (type == EventType::SynthParam) {
        std::string synthParamStr = pt.GetString("synth_param");
        param = StringToSynthParamType(synthParamStr.c_str());
        newParamValue = pt.GetDouble("value");
    } else {
        midiNote = pt.GetInt("midi_note");
        velocity = pt.GetFloat("velocity");
    }
}