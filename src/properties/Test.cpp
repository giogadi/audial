#include "src/properties/Test.h"

#include "imgui/imgui.h"

#include "imgui_util.h"


#include "imgui_vector_util.h"

#include "serial_vector_util.h"


void TestProps::Load(serial::Ptree pt) {
   
   pt.TryGetInt("myInt1", &_myInt1);
   
   pt.TryGetInt("myInt2", &_myInt2);
   
   pt.TryGetString("myString", &_myString);
   
   serial::LoadVectorFromChildNode<float>(pt, "myFloatArray", _myFloatArray);
   
   serial::TryGetEnum(pt, "mySeqActionType", _mySeqActionType);
   
   serial::LoadFromChildOf(pt, "myVec3", _myVec3);
   
}

void TestProps::Save(serial::Ptree pt) const {
    
    pt.PutInt("myInt1", _myInt1);
    
    pt.PutInt("myInt2", _myInt2);
    
    pt.PutString("myString", _myString.c_str());
    
    serial::SaveVectorInChildNode<float>(pt, "myFloatArray", "array_item", _myFloatArray);
    
    serial::PutEnum(pt, "mySeqActionType", _mySeqActionType);
    
    serial::SaveInNewChildOf(pt, "myVec3", _myVec3);
    
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
    
    {
        bool thisChanged = SeqActionTypeImGui("SeqActionType", &_mySeqActionType);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputVec3("myVec3", &_myVec3);
        changed = changed || thisChanged;
    }
    
    return changed;
}