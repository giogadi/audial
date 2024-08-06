#pragma once


#include "editor_id.h"

#include "properties/MidiNoteAndName.h"

#include <vector>


#include "serial.h"

struct ChangeStepSequencerSeqActionProps {
    
    EditorId _seqEntityEditorId;
    
    bool _velOnly;
    
    bool _temporary;
    
    std::vector<MidiNoteAndName> _midiNotes;
    
    float _velocity;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};