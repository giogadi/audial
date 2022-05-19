#pragma once

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"

#include "src/enums/CollisionLayer.h"



template<typename Archive>
std::string save_minimal(Archive const& ar, CollisionLayer const& e) {
	return std::string(CollisionLayerToString(e));
}

template<typename Archive>
void load_minimal(Archive const& ar, CollisionLayer& e, std::string const& v) {
	e = StringToCollisionLayer(v.c_str());
}

