#pragma once


#include <vector>

#include "matrix.h"

#include "enums/SeqActionType.h"


#include "serial.h"

struct TestProps {
    
    int _myInt1;
    
    int _myInt2;
    
    std::string _myString;
    
    std::vector<float> _myFloatArray;
    
    SeqActionType _mySeqActionType;
    
    Vec3 _myVec3;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};