#include "entities/enemy.h"

#include <array>
#include <sstream>

#include "imgui/imgui.h"
#include "game_manager.h"
#include "sound_bank.h"
#include "serial.h"
#include "beat_time_event.h"
#include "audio.h"

namespace {

std::array<char,1024> gAudioEventScriptBuf;

audio::Event GetEventAtBeatOffsetFromNextDenom(double denom, BeatTimeEvent const& b_e, BeatClock const& beatClock) {
    double beatTime = beatClock.GetBeatTime();
    double startTime = BeatClock::GetNextBeatDenomTime(beatTime, denom);
    unsigned long startTickTime = beatClock.BeatTimeToTickTime(startTime);
    audio::Event e = b_e._e;
    e.timeInTicks = beatClock.BeatTimeToTickTime(b_e._beatTime) + startTickTime;
    return e;
}

void ReadFromScript(std::vector<BeatTimeEvent>& events, SoundBank const& soundBank) {
    events.clear();

    std::string scriptStr(gAudioEventScriptBuf.data());
    std::stringstream ss(scriptStr);
    std::string line;
    std::string token;        
    BeatTimeEvent b_e;
    while (!ss.eof()) {
        std::getline(ss, line);
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

void WriteToScript(std::vector<BeatTimeEvent> const& events, SoundBank const& soundBank) {
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
    if (result.length() >= gAudioEventScriptBuf.size()) {
        printf("WARNING: AUDIO SCRIPT WAS CUTOFF\n");
    }
    strncpy(gAudioEventScriptBuf.data(), result.c_str(), gAudioEventScriptBuf.size());
}

} // end namespace

// TODO: consider using the text format we made above for this.
void EnemyEntity::SaveDerived(serial::Ptree pt) const {
    serial::Ptree eventsPt = pt.AddChild("events");
    for (BeatTimeEvent const& b_e : _events) {
        serial::Ptree eventPt = eventsPt.AddChild("beat_event");
        b_e.Save(eventPt);
    }
}

void EnemyEntity::LoadDerived(serial::Ptree pt) {
    serial::Ptree eventsPt = pt.TryGetChild("events");
    if (eventsPt.IsValid()) {
        int numChildren = 0;
        serial::NameTreePair* children = eventsPt.GetChildren(&numChildren);
        for (int i = 0; i < numChildren; ++i) {
            _events.emplace_back();
            _events.back().Load(children[i]._pt);
        }
        delete[] children;
    }    
}

ne::Entity::ImGuiResult EnemyEntity::ImGuiDerived(GameManager& g) {
    ImGui::InputDouble("Event start denom", &_eventStartDenom);
    ImGui::InputTextMultiline("Audio events", gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    if (ImGui::Button("Apply Script to Entity")) {
        ReadFromScript(_events, *g._soundBank);
    }
    if (ImGui::Button("Apply Entity to Script")) {
        WriteToScript(_events, *g._soundBank);
    }
    return ImGuiResult::Done;
}

void EnemyEntity::OnEditPick(GameManager& g) {
    SendEvents(g);
}

void EnemyEntity::SendEvents(GameManager& g) {
    for (BeatTimeEvent const& b_e : _events) {
        audio::Event e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock);
        g._audioContext->AddEvent(e);
    }    
}