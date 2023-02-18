#pragma once

#include "imgui/imgui.h"

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

inline bool InputVec3(char const* label, Vec3* v, bool trueOnReturnOnly = false) {
    float posData[3];
    v->CopyToArray(posData);
    bool changed = ImGui::InputFloat3(label, posData, "%.3f", trueOnReturnOnly ? ImGuiInputTextFlags_EnterReturnsTrue : 0);
    if (changed) {
        *v = Vec3(posData);
    }
    return changed;
}

inline bool ColorEdit3(char const* label, Vec3* v) {
    float data[3];
    v->CopyToArray(data);
    bool changed = ImGui::ColorEdit3(label, data);
    if (changed) {
        *v = Vec3(data);
    }
    return changed;
}

inline bool ColorEdit4(char const* label, Vec4* v) {
    float data[4];
    v->CopyToArray(data);
    bool changed = ImGui::ColorEdit4(label, data);
    if (changed) {
        *v = Vec4(data);
    }
    return changed;
}

}
