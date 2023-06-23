#pragma once



#include "serial.h"

struct VfxEntityProps {
    
    float _cycleTimeSecs = 0.2f;
    
    float _pulseScaleFactor = 1.1f;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};