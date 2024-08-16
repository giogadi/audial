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
    char buttonLabelBuffer[256];
    char popupLabelBuffer[256];
    snprintf(buttonLabelBuffer, 256, "Browse##%s", label);
    snprintf(popupLabelBuffer, 256, "EntityPopup##%s", label);
    ne::Entity* newSelection = gGameManager._editor->ImGuiEntitySelector(buttonLabelBuffer, popupLabelBuffer);
    ImGui::SameLine();
    if (e == nullptr) {
        ImGui::BeginDisabled(true);
    }
	snprintf(buttonLabelBuffer, 256, "Go to##%s", label);
    if (ImGui::Button(buttonLabelBuffer)) {
        assert(e);
        gGameManager._editor->SelectEntity(*e);
    }
    if (e == nullptr) {
        ImGui::EndDisabled();
    }
	if (newSelection != nullptr) {
		*v = newSelection->_editorId;
        result = true;
	}
	ImGui::SameLine();
	snprintf(buttonLabelBuffer, 256, "Clear##%s", label);
	if (ImGui::Button(buttonLabelBuffer)) {
		*v = EditorId();
		result = true;
	}
	return result;
}

}
