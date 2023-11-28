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
    bool entered = ImGui::InputInt("MIDI", &_note, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (entered) {
        GetNoteName(_note, _name);
    }
    entered = ImGui::InputText("Name", _name, 4, ImGuiInputTextFlags_EnterReturnsTrue);
    if (entered) {
        _note = GetMidiNote(_name);
    }

    return entered;
}

void MidiNoteAndName::UpdateName() {
    GetNoteName(_note, _name);
}