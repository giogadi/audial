#pragma once


#include "properties/MidiNoteAndName.h"

#include <vector>

#include "editor_id.h"


#include "serial.h"

struct SetAllStepsSeqActionProps {
    
    EditorId _seqEntityEditorId;
    
    bool _velOnly;
    
    std::vector<MidiNoteAndName> _midiNotes;
    
    float _velocity;
    
    int _trackIx = -1;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};