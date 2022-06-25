#include "audio_util.h"

void audio::Event::Save(ptree& pt) const {
    std::string eventTypeStr = EventTypeToString(type);
    pt.put("event_type", eventTypeStr);
    pt.put("channel", channel);
    pt.put("tick_time", timeInTicks);
    if (type == EventType::SynthParam) {
        pt.put("synth_param", SynthParamTypeToString(param));
        pt.put("value", newParamValue);
    } else {
        pt.put("midi_note", midiNote);
        pt.put("velocity", velocity);
    }
}
void audio::Event::Load(ptree const& pt) {
    std::string eventTypeStr = pt.get<std::string>("event_type");
    type = StringToEventType(eventTypeStr.c_str());
    channel = pt.get<int>("channel");
    timeInTicks = pt.get<long>("tick_time");
    if (type == EventType::SynthParam) {
        std::string synthParamStr = pt.get<std::string>("synth_param");
        param = StringToSynthParamType(synthParamStr.c_str());
        newParamValue = pt.get<double>("value");
    } else {
        midiNote = pt.get<int>("midi_note");
        velocity = pt.get<float>("velocity");
    }
}