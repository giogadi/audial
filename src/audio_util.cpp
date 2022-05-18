#include "audio_util.h"

#include <unordered_map>
#include <string>

namespace audio {

namespace {

char const* gEventTypeStrings[] = {
#   define X(a) #a,
    M_AUDIO_EVENT_TYPES
#   undef X
};

std::unordered_map<std::string, EventType> const gStringToEventType = {
#   define X(name) {#name, EventType::name},
    M_AUDIO_EVENT_TYPES
#   undef X
};

char const* gSynthParamTypeStrings[] = {
#   define X(a) #a,
    M_AUDIO_SYNTH_PARAM_TYPES
#   undef X
};

std::unordered_map<std::string, SynthParamType> const gStringToSynthParamType = {
#   define X(name) {#name, SynthParamType::name},
    M_AUDIO_SYNTH_PARAM_TYPES
#   undef X
};

}  // namespace

char const* EventTypeToString(EventType c) {
    return gEventTypeStrings[static_cast<int>(c)];
}

EventType StringToEventType(char const* c) {
    return gStringToEventType.at(c);
}

char const* SynthParamTypeToString(SynthParamType c) {
    return gSynthParamTypeStrings[static_cast<int>(c)];
}

SynthParamType StringToSynthParamType(char const* c) {
    return gStringToSynthParamType.at(c);
}

}  // namespace audio