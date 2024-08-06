#pragma once


#include <vector>

#include "properties/MidiNoteAndName.h"


#include "serial.h"

struct NoteOnOffSeqActionProps {
    
    int _channel = 0;
    
    std::vector<MidiNoteAndName> _midiNotes;
    
    double _noteLength = 0.0;
    
    float _velocity = 1.f;
    
    double _quantizeDenom = 0.25;
    
    bool _doNoteOff = true;
    
    bool _holdNotes = false;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};