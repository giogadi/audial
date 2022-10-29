#pragma once

namespace imgui_util {
    template<int N>
    inline bool InputText(char const* label, std::string* value, bool trueOnReturnOnly = false) {
        char buf[N];
        strncpy(buf, value->c_str(), N);
        bool changed = ImGui::InputText(label, buf, N, trueOnReturnOnly ? ImGuiInputTextFlags_EnterReturnsTrue : 0);
        if (changed) {
            *value = buf;
        }
        return changed;
    }
}