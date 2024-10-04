#include "audio_util.h"

void audio::Event::Save(serial::Ptree pt) const {
    std::string eventTypeStr = EventTypeToString(type);
    pt.PutString("event_type", eventTypeStr.c_str());
    pt.PutInt("channel", channel);
    pt.PutDouble("delay_secs", delaySecs);    
    switch (type) {
        case EventType::SynthParam:
            pt.PutString("synth_param", SynthParamTypeToString(param));
            pt.PutDouble("change_time_secs", paramChangeTimeSecs);
            // THIS IS BROKEN AND BADDDDDDDDDDDDD not handling int case
            pt.PutFloat("value", newParamValue);
            break;
        case EventType::PlayPcm:
            pt.PutInt("sound_ix", pcmSoundIx);
            pt.PutFloat("velocity", velocity);
            pt.PutBool("loop", loop);
            break;
        case EventType::StopPcm:
            pt.PutInt("sound_ix", pcmSoundIx);
            break;
        case EventType::NoteOn:
            pt.PutInt("midi_note", midiNote);
            pt.PutFloat("velocity", velocity);
            pt.PutInt("prime_porta", primePortaMidiNote);
            break;
        case EventType::NoteOff:
            pt.PutInt("midi_note", midiNote);
            break;
        case EventType::AllNotesOff:
            break;
        case EventType::SetGain:
            pt.PutFloat("gain", newGain);
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
    if (!pt.TryGetDouble("delay_secs", &delaySecs)) {
        // backward compat. DIRTY HACK, assume 48khz sample rate.        
        int64_t tickTime = pt.GetInt64("tick_time");
        printf("audio::Event::Load: DIRTY HACK AHOY: %lld\n", tickTime);
        delaySecs = static_cast<double>(tickTime) / 48000.0;
    }
    switch (type) {
        case EventType::SynthParam: {
            std::string synthParamStr = pt.GetString("synth_param");
            param = StringToSynthParamType(synthParamStr.c_str());
            paramChangeTimeSecs = 0.0;
            if (!pt.TryGetDouble("change_time_secs", &paramChangeTimeSecs)) {
                int64_t tickTime = pt.GetInt64("change_time");
                printf("audio::Event::Load: DIRTY HACK PARAM AHOY: %lld\n", tickTime);
                paramChangeTimeSecs = static_cast<double>(tickTime) / 48000.0;
            }
            // THIS IS BROKEN AND BADDDDDDDDDDDDD not handling int case
            newParamValue = pt.GetFloat("value");            
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
        case EventType::StopPcm:
            pcmSoundIx = pt.GetInt("sound_ix");
            break;
        case EventType::NoteOn:
            midiNote = pt.GetInt("midi_note");
            velocity = pt.GetFloat("velocity");
            primePortaMidiNote = -1;
            pt.TryGetInt("prime_porta", &primePortaMidiNote);
            break;
        case EventType::NoteOff:
            midiNote = pt.GetInt("midi_note");
            break;
        case EventType::AllNotesOff:
            break;
        case EventType::SetGain:
            newGain = pt.GetFloat("gain");
            break;
        case EventType::None:
        case EventType::Count:
            break;
    }
}
