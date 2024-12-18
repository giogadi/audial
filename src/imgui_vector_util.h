#pragma once

#include <vector>

#include "imgui/imgui.h"
#include "imgui_util.h"
#include "new_entity.h"

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
    return imgui_util::InputText<512>("", &v);
}

template <>
inline bool ImGuiElement<Vec3>(Vec3& v) {
    return imgui_util::InputVec3("", &v);
}

template <>
inline bool ImGuiElement<EditorId>(EditorId& v) {
    return imgui_util::InputEditorId("", &v);
}

struct InputVectorOptions {
    bool treePerItem = false;
    bool removeOnSameLine = false;
};

template <typename T>
inline bool InputVector(std::vector<T>& v, InputVectorOptions options = InputVectorOptions()) {
    bool changed = false;
    if (ImGui::Button("Add")) {
        v.emplace_back();
        changed = true;
    }
    int deleteIx = -1;
    char labelBuf[32];
    for (int i = 0, n = v.size(); i < n; ++i) {
        sprintf(labelBuf, "Item %d", i);
        bool treePushed = false;
        if (options.treePerItem) {
            treePushed = ImGui::TreeNode(labelBuf);
        } else {
            ImGui::PushID(labelBuf);
        }
        if (!options.treePerItem || treePushed) {
            bool thisChanged = ImGuiElement(v[i]);
            changed = changed || thisChanged;
            if (options.removeOnSameLine) {
                ImGui::SameLine();
            }
            if (ImGui::Button("Remove")) {
                deleteIx = i;
                changed = true;
            }
        }
        if (options.treePerItem) {
            if (treePushed) {
                ImGui::TreePop();
            }
        } else {
            ImGui::PopID();
        } 
    }

    if (deleteIx >= 0) {
        v.erase(v.begin() + deleteIx);
    }

    return changed;
}

template <typename MemberType, typename StructType>
void SetMemberOfStructs(MemberType newValue, MemberType(StructType::*memberPtr), StructType *elements, std::size_t elementCount) {
    for (int ii = 0; ii < elementCount; ++ii) {
        StructType &e = elements[ii];
        (e.*memberPtr) = newValue;
    }
}

template <typename TMemberType, typename TEntityType>
void SetMemberOfEntities(TMemberType(TEntityType::*memberPtr), TEntityType const &example, ne::BaseEntity **entities, std::size_t entityCount) {
    for (int ii = 0; ii < entityCount; ++ii) {
        ne::BaseEntity *base = entities[ii];
        TEntityType *e = base->As<TEntityType>();
        (e->*memberPtr) = (example.*memberPtr);
    }
}

template <typename TMemberType, typename TEntityType>
void SetMemberOfEntities(TMemberType(TEntityType::Props::*memberPtr), TEntityType const &example, ne::BaseEntity **entities, std::size_t entityCount) {
    for (int ii = 0; ii < entityCount; ++ii) {
        ne::BaseEntity *base = entities[ii];
        TEntityType *e = base->As<TEntityType>();
        assert(e);
        e->_p.*memberPtr = example._p.*memberPtr;
    }
}

}