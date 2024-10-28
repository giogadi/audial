#include "editor_id.h"

#include "string_util.h"

EditorIdMap *gEditorIdMap = nullptr;

std::string EditorId::ToString() const {
	return std::to_string(_id);
}

bool EditorId::SetFromString(std::string const& s) {
	bool success = string_util::MaybeStoi64(s, _id);
	return success;
}

void EditorId::Load(serial::Ptree pt) {
    _id = pt.GetInt64("id");
    if (gEditorIdMap) {
        auto iter = gEditorIdMap->find(_id);
        if (iter != gEditorIdMap->end()) {
            _id = iter->second;
        }
    }
}

