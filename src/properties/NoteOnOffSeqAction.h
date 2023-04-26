#pragma once



#include "serial.h"

struct NoteOnOffSeqActionProps {
    
    int _channel = 0;
    
    int _midiNote = -1;
    
    double _noteLength = 0.0;
    
    float _velocity = 1.f;
    
    double _quantizeDenom = 0.25;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};