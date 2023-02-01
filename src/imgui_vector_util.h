#pragma once

#include <vector>

#include "imgui/imgui.h"

namespace imgui_util {

template <typename T>
bool InputVector(std::vector<T>& v) {
    if (ImGui::Button("Add")) {
        v.emplace_back();
    }
    int deleteIx = -1;
    bool overallNeedInit = false;
    for (int i = 0, n = v.size(); i < n; ++i) {
        ImGui::PushID(i);
        bool thisNeedInit = v[i].ImGui();
        if (thisNeedInit) {
            overallNeedInit = true;
        }
        if (ImGui::Button("Remove")) {
            deleteIx = i;
            overallNeedInit = true;
        }
        ImGui::PopID();
    }

    if (deleteIx >= 0) {
        v.erase(v.begin() + deleteIx);
    }
}

}
