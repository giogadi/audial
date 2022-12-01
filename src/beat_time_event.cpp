#include "beat_time_event.h"

#include <sstream>
#include <iostream>

#include "sound_bank.h"
#include "midi_util.h"

void ReadBeatEventsFromScript(
    std::vector<BeatTimeEvent>& events, SoundBank const& soundBank,
    char const* scriptBuf, int scriptBufLength) {
    events.clear();

    std::string scriptStr(scriptBuf);
    std::stringstream ss(scriptStr);
    std::string line;
    std::string token;        
    BeatTimeEvent b_e;
    while (!ss.eof()) {
        std::getline(ss, line);
        // If it's only whitespace, just skip.
        bool onlySpaces = true;
        for (char c : line) {
            if (!isspace(c)) {
                onlySpaces = false;
                break;
            }
        }
        if (onlySpaces) {
            continue;
        }
        std::stringstream lineStream(line);

        while (!lineStream.eof()) {

            std::string key, value;
            {
                lineStream >> token;
                std::size_t delimIx = token.find_first_of(':');
                if (delimIx == std::string::npos) {
                    printf("Token missing \":\" - \"%s\"\n", token.c_str());
                    continue;
                }

                key = token.substr(0, delimIx);
                value = token.substr(delimIx+1);
            }            

            if (key == "e") {
                // event type.
                if (value == "on") {
                    b_e._e.type = audio::EventType::NoteOn;
                } else if (value == "off") {
                    b_e._e.type = audio::EventType::NoteOff;
                } else if (value == "alloff") {
                    b_e._e.type = audio::EventType::AllNotesOff;
                } else if (value == "play") {
                    b_e._e.type = audio::EventType::PlayPcm;
                } else if (value == "stop") {
                    b_e._e.type = audio::EventType::StopPcm;
                } else if (value == "none") {
                    b_e._e.type = audio::EventType::None;
                } else {
                    printf("Unrecognized audio event type: %s\n", value.c_str());
                    break;
                }
            } else if (key == "c") {
                // channel
                try {
                    b_e._e.channel = std::stoi(value);
                } catch (std::exception& e) {
                    printf("Failed to parse channel: %s\n", value.c_str());
                    break;
                }
            } else if (key == "t") {
                // beat time
                try {
                    b_e._beatTime = std::stof(value);
                } catch (std::exception& e) {
                    printf("Failed to parse beat time: %s\n", value.c_str());
                    break;
                }
            } else if (key == "m") {
                // midi note
                try {
                    b_e._e.midiNote = std::stoi(value);
                } catch (std::exception& e) {
                    printf("Failed to parse midi note: %s\n", value.c_str());
                    break;
                }
            } else if (key == "mm") {
                // midi note (string rep)
                b_e._e. midiNote = GetMidiNote(value.c_str());
                if (b_e._e.midiNote < 0) {
                    printf("Failed to parse midi string: %s\n", value.c_str());
                    break;
                }
            } else if (key == "s") {
                int soundIx = soundBank.GetSoundIx(value.c_str());
                if (soundIx < 0) {
                    printf("Failed to find sound \"%s\"\n", value.c_str());
                    break;
                }
                b_e._e.pcmSoundIx = soundIx;
            } else if (key == "v") {
                // velocity
                float v;
                try {
                    v = std::stof(value);
                } catch (std::exception& e) {
                    printf("Failed to parse velocity: %s\n", value.c_str());
                    break;
                }
                // GROSSSSSSSSS
                switch (b_e._e.type) {
                    case audio::EventType::NoteOn: {
                        b_e._e.velocity = v;
                        break;
                    }
                    case audio::EventType::PlayPcm: {
                        b_e._e.pcmVelocity = v;
                        break;
                    }
                    default: {
                        printf("Tried to set velocity of an audio event that doesn't take velocity.\n");
                        break;
                    }
                }
            } else if (key == "l") {
                if (b_e._e.type != audio::EventType::PlayPcm) {
                    printf("Tried to set loop on an audio event that doesn't take loop.\n");
                    break;
                }
                try {
                    b_e._e.loop = std::stoi(value);
                } catch (std::exception& e) {
                    printf("Failed to parse loop setting: %s\n", value.c_str());
                    break;
                }
            }
        }        

        events.push_back(b_e);
    }
}

void WriteBeatEventsToScript(
    std::vector<BeatTimeEvent> const& events, SoundBank const& soundBank,
    char* scriptBuf, int scriptBufLength) {
    std::stringstream ss;
    for (BeatTimeEvent const& b_e : events) {
        switch (b_e._e.type) {
            case audio::EventType::NoteOn:
                ss << "e:on ";
                break;
            case audio::EventType::NoteOff:
                ss << "e:off ";
                break;
            case audio::EventType::AllNotesOff:
                ss << "e:alloff ";
                break;
            case audio::EventType::PlayPcm:
                ss << "e:play ";
                break;
            case audio::EventType::StopPcm:
                ss << "e:stop ";
                break;
            case audio::EventType::None:
                ss << "e:none ";
                break;
            default:
                printf("unsupported event type %s\n", audio::EventTypeToString(b_e._e.type));
                continue;
        }        

        ss << "t:" << b_e._beatTime << " ";

        if (b_e._e.type == audio::EventType::NoteOn || b_e._e.type == audio::EventType::NoteOff) {
            ss << "c:" << b_e._e.channel << " ";
            ss << "m:" << b_e._e.midiNote << " ";
        }

        if (b_e._e.type == audio::EventType::PlayPcm || b_e._e.type == audio::EventType::StopPcm) {
            char const* soundName = soundBank._soundNames[b_e._e.pcmSoundIx];
            ss << "s:" << soundName << " ";
        }

        if (b_e._e.type == audio::EventType::PlayPcm) {
            ss << "v:" << b_e._e.pcmVelocity << " ";
            int loopVal = (int) b_e._e.loop;
            ss << "l:" << loopVal << " ";
        }

        if (b_e._e.type == audio::EventType::NoteOn) {
            ss << "v:" << b_e._e.velocity << " ";
        }

        ss << std::endl;
    }
    std::string result = ss.str();
    if (result.length() >= scriptBufLength) {
        printf("WARNING: AUDIO SCRIPT WAS CUTOFF\n");
    }
    strncpy(scriptBuf, result.c_str(), scriptBufLength);
}

audio::Event GetEventAtBeatOffsetFromNextDenom(
    double denom, BeatTimeEvent const& b_e, BeatClock const& beatClock, double slack) {
    double beatTime = beatClock.GetBeatTime();
    double startTime = BeatClock::GetNextBeatDenomTime(beatTime, denom);
    double prevDenom = startTime - denom;
    double timeSincePrevDenom = beatTime - prevDenom;
    if (timeSincePrevDenom >= 0.0 && timeSincePrevDenom <= slack) {
        startTime = beatTime;
    }
    unsigned long startTickTime = beatClock.BeatTimeToTickTime(startTime);
    audio::Event e = b_e._e;
    e.timeInTicks = beatClock.BeatTimeToTickTime(b_e._beatTime) + startTickTime;
    return e;
}
