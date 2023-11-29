#pragma once


#include <vector>

#include "properties/MidiNoteAndName.h"

#include "editor_id.h"


#include "serial.h"

struct SetAllStepsSeqActionProps {
    
    EditorId _seqEntityEditorId;
    
    bool _velOnly;
    
    std::vector<MidiNoteAndName> _midiNotes;
    
    float _velocity;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};