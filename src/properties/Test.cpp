#include "src/properties/Test.h"

#include "imgui/imgui.h"

#include "imgui_util.h"



void TestProps::Load(serial::Ptree pt) {
   
   pt.TryGetInt("myInt1", &_myInt1);
   
   pt.TryGetInt("myInt2", &_myInt2);
   
   pt.TryGetString("myString", &_myString);
   
   serial::LoadVectorFromChildNode<float>(pt, "myFloatArray", _myFloatArray);
   
}

void TestProps::Save(serial::Ptree pt) const {
    
    pt.PutInt("myInt1", _myInt1);
    
    pt.PutInt("myInt2", _myInt2);
    
    pt.PutString("myString", _myString.c_str());
    
    serial::SaveVectorFromChildNode<float>(pt, "myFloatArray", "array_item", _myFloatArray);
    
}

bool TestProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = ImGui::InputInt("myInt1", &_myInt1);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputInt("myInt2", &_myInt2);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputText<128>("myString", &_myString);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputVector(_myFloatArray);
        changed = changed || thisChanged;
    }
    
    return changed;
}