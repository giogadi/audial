#pragma once



#include "serial.h"

struct ChangeStepSeqMaxVoicesProps {
    
    std::string _entityName;
    
    bool _relative = false;
    
    int _numVoices = 0;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};