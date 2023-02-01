#pragma once

#include "serial.h"

struct TestProps {
    
    int _myInt1;
    
    int _myInt2;
    
    std::string _myString;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};