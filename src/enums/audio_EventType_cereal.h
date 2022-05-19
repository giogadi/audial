#pragma once

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"

#include "src/enums/audio_EventType.h"


namespace audio {


template<typename Archive>
std::string save_minimal(Archive const& ar, EventType const& e) {
	return std::string(EventTypeToString(e));
}

template<typename Archive>
void load_minimal(Archive const& ar, EventType& e, std::string const& v) {
	e = StringToEventType(v.c_str());
}


}
