#include "imgui_util.h"

#include "game_manager.h"
#include "new_entity.h"
#include "editor.h"

extern GameManager gGameManager;

namespace imgui_util {

bool InputEditorId(char const* label, EditorId* v) {	
	ImGui::PushItemWidth(50);
	bool result = ImGui::InputScalar(label, ImGuiDataType_S64, &(v->_id), 0, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ne::Entity* e = gGameManager._neEntityManager->FindEntityByEditorId(*v);
	if (e) {
		ImGui::Text("%s (%s)", e->_name.c_str(), ne::gkEntityTypeNames[(int)e->_id._type]);
	} else {
		ImGui::Text("<Entity not found>");
	}
	e = gGameManager._editor->ImGuiEntitySelector("Browse", "EntityPopup");
	if (e != nullptr) {
		*v = e->_editorId;
	}
	return result;
}

}