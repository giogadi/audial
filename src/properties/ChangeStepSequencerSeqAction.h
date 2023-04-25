#pragma once



#include "serial.h"

struct ChangeStepSequencerSeqActionProps {
    
    std::string _seqName;
    
    bool _velOnly;
    
    bool _temporary;
    
    std::string _stepStr;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};