#pragma once

#include <vector>

#include "imgui/imgui.h"
#include "imgui_util.h"

namespace imgui_util {

template <typename T>
inline bool ImGuiElement(T& v) {
    return v.ImGui();
}

template <>
inline bool ImGuiElement<int>(int& v) {
    return ImGui::InputInt("", &v);
}

template <>
inline bool ImGuiElement<bool>(bool& v) {
    return ImGui::Checkbox("", &v);
}

template <>
inline bool ImGuiElement<float>(float& v) {
    return ImGui::InputFloat("", &v);
}

template <>
inline bool ImGuiElement<double>(double& v) {
    return ImGui::InputDouble("", &v);
}

template <>
inline bool ImGuiElement<std::string>(std::string& v) {
    return imgui_util::InputText("", &v);
}

template <typename T>
inline bool InputVector(std::vector<T>& v) {
    bool changed = false;
    if (ImGui::Button("Add")) {
        v.emplace_back();
        changed = true;
    }
    int deleteIx = -1;
    for (int i = 0, n = v.size(); i < n; ++i) {
        ImGui::PushID(i);
        bool thisChanged = ImGuiElement(v[i]);
        changed = changed || thisChanged;
        if (ImGui::Button("Remove")) {
            deleteIx = i;
            changed = true;
        }
        ImGui::PopID();
    }

    if (deleteIx >= 0) {
        v.erase(v.begin() + deleteIx);
    }

    return changed;
}

}
