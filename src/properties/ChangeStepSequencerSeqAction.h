#pragma once


#include "properties/MidiNoteAndName.h"

#include "editor_id.h"

#include <vector>


#include "serial.h"

struct ChangeStepSequencerSeqActionProps {
    
    EditorId _seqEntityEditorId;
    
    bool _velOnly;
    
    bool _temporary;
    
    std::vector<MidiNoteAndName> _midiNotes;
    
    float _velocity;
    
    float _gain = -1.f;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};