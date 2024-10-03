#pragma once


#include "properties/MidiNoteAndName.h"

#include "editor_id.h"

#include "properties/SynthParamValue.h"

#include <vector>


#include "serial.h"

struct ChangeStepSequencerSeqActionProps {
    
    EditorId _seqEntityEditorId;
    
    bool _changeVel;
    
    bool _changeNote;
    
    bool _temporary;
    
    std::vector<MidiNoteAndName> _midiNotes;
    
    float _velocity;
    
    std::vector<SynthParamValue> _params;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};