#pragma once

#include "serial.h"

struct MidiNoteAndName {
	int _note = -1;
	char _name[4] = "";

	MidiNoteAndName() {}
	explicit MidiNoteAndName(int note) : _note(note) { UpdateName(); }
	void UpdateName();

	void Save(serial::Ptree pt) const;
	void Load(serial::Ptree pt);
	bool ImGui();
};