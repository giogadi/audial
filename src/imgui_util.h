#pragma once

namespace imgui_util {
    template<int N>
    inline bool InputText(char const* label, std::string* value) {
        char buf[N];
        strncpy(buf, value->c_str(), N);
        bool changed = ImGui::InputText(label, buf, N);
        if (changed) {
            *value = buf;
        }
        return changed;
    }
}