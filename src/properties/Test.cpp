#include "src/properties/Test.h"

#include "imgui/imgui.h"

#include "imgui_util.h"

// TODO HEADERS

void TestProps::Load(serial::Ptree pt) {
   
   pt.TryGetInt("myInt1", &_myInt1);
   
   pt.TryGetInt("myInt2", &_myInt2);
   
   pt.TryGetString("myString", &_myString);
   
}

void TestProps::Save(serial::Ptree pt) const {
    
    pt.PutInt("myInt1", _myInt1);
    
    pt.PutInt("myInt2", _myInt2);
    
    pt.PutString("myString", _myString.c_str());
    
}

bool TestProps::ImGui() {
    bool overallNeedsInit = false;
    bool thisNeedsInit = false;
    
    thisNeedsInit = ImGui::InputInt("myInt1", &_myInt1);
    overallNeedsInit = thisNeedsInit || overallNeedsInit;
    
    thisNeedsInit = ImGui::InputInt("myInt2", &_myInt2);
    overallNeedsInit = thisNeedsInit || overallNeedsInit;
    
    thisNeedsInit = imgui_util::InputText<128>("myString", &_myString);
    overallNeedsInit = thisNeedsInit || overallNeedsInit;
    
    return overallNeedsInit;
}