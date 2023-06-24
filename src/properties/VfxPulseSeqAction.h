#pragma once



#include "serial.h"

struct VfxPulseSeqActionProps {
    
    std::string _vfxEntityName;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};