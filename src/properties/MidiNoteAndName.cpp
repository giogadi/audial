#include "MidiNoteAndName.h"

#include "imgui/imgui.h"

#include "midi_util.h"

void MidiNoteAndName::Save(serial::Ptree pt) const {
	pt.PutInt("note", _note);
}

void MidiNoteAndName::Load(serial::Ptree pt) {
	_note = pt.GetInt("note");
    UpdateName();
}

bool MidiNoteAndName::ImGui() {
    float const fontSize = ImGui::GetFontSize();
    ImGui::PushItemWidth(fontSize * 8);
    bool entered = ImGui::InputInt("MIDI", &_note, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(fontSize * 4);
    ImGui::SameLine();
    if (entered) {
        GetNoteName(_note, _name);
    }
    entered = ImGui::InputText("Name", _name, 4, ImGuiInputTextFlags_EnterReturnsTrue);
    if (entered) {
        _note = GetMidiNote(_name);
    }

    ImGui::PopItemWidth();
    return entered;
}

void MidiNoteAndName::UpdateName() {
    GetNoteName(_note, _name);
}
