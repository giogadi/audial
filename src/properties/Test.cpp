#include "src/properties/Test.h"

#include "imgui/imgui.h"

#include "imgui_util.h"

void TestProps::Load(serial::Ptree pt) {
    
    
    pt.TryGetInt("myInt1", &_myInt1);
    
    
    
    pt.TryGetInt("myInt2", &_myInt2);
    
    
    
    pt.TryGetString("myString", &_myString);
    
    
}

void TestProps::Save(serial::Ptree pt) const {
     
     
     pt.PutInt("myInt1", _myInt1);
     
     
     
     pt.PutInt("myInt2", _myInt2);
     
     
     
     pt.PutString("myString", _myString);
     
     
}

bool TestProps::ImGui() {
     
     
     ImGui::InputInt("myInt1", &_myInt1);
     
     
     
     ImGui::InputInt("myInt2", &_myInt2);
     
     
     
     imgui_util::InputText<128>("myString", &_myString);
     
     
}