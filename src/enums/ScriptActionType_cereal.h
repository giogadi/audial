#pragma once

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"

#include "src/enums/ScriptActionType.h"



template<typename Archive>
std::string save_minimal(Archive const& ar, ScriptActionType const& e) {
	return std::string(ScriptActionTypeToString(e));
}

template<typename Archive>
void load_minimal(Archive const& ar, ScriptActionType& e, std::string const& v) {
	e = StringToScriptActionType(v.c_str());
}

