#include "editor.h"
#include "string_util.h"

std::string EditorId::ToString() const {
	return std::to_string(_id);
}

bool EditorId::SetFromString(std::string const& s) {
	bool success = string_util::MaybeStoi64(s, _id);
	return success;
}