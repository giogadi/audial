#pragma once

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"

#include "src/enums/audio_SynthParamType.h"


namespace audio {


template<typename Archive>
std::string save_minimal(Archive const& ar, SynthParamType const& e) {
	return std::string(SynthParamTypeToString(e));
}

template<typename Archive>
void load_minimal(Archive const& ar, SynthParamType& e, std::string const& v) {
	e = StringToSynthParamType(v.c_str());
}


}
