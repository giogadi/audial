#pragma once

namespace imgui_util {
    inline bool InputText128(char const* label, std::string* value) {
        char buf[128];
        strncpy(buf, value->c_str(), 128);        
        bool changed = ImGui::InputText(label, buf, 128);
        if (changed) {
            *value = buf;
        }
        return changed;
    }
}