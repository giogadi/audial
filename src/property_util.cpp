#include "property_util.h"

#include "imgui/imgui.h"

bool ImGuiNodeStart(char const* label) {
    return ImGui::TreeNode(label);
}

void ImGuiNodePop() {
    return ImGui::TreePop();
}
