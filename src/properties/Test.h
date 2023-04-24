#pragma once

#include "serial.h"


#include "enums/SeqActionType.h"

#include <vector>


struct TestProps {
    
    int _myInt1;
    
    int _myInt2;
    
    std::string _myString;
    
    std::vector<float> _myFloatArray;
    
    SeqActionType _mySeqActionType;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};