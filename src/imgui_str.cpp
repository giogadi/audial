#include "imgui_str.h"

#include <imgui/imgui.h>

namespace imgui_util {

void Char::Save(serial::Ptree pt, char const* name) const {
    pt.PutChar(name, c);
}

void Char::Load(serial::Ptree pt, char const* name) {
    pt.TryGetChar(name, &c);
    bufInternal[0] = c;
}

bool Char::ImGui(char const* name) {
    if (ImGui::InputText(name, bufInternal, 2, 0/*ImGuiInputTextFlags_EnterReturnsTrue*/)) {
        if (bufInternal[0] != '\0') {
            c = bufInternal[0]; 
            return true;
        } else {
            bufInternal[0] = c;
            bufInternal[1] = '\0';
        }
    }
    return false;
}

} 
