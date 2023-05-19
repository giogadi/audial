#include "src/properties/ChangeStepSeqMaxVoices.h"

#include "imgui/imgui.h"

#include "imgui_util.h"



void ChangeStepSeqMaxVoicesProps::Load(serial::Ptree pt) {
   
   pt.TryGetString("entityName", &_entityName);
   
   pt.TryGetBool("relative", &_relative);
   
   pt.TryGetInt("numVoices", &_numVoices);
   
}

void ChangeStepSeqMaxVoicesProps::Save(serial::Ptree pt) const {
    
    pt.PutString("entityName", _entityName.c_str());
    
    pt.PutBool("relative", _relative);
    
    pt.PutInt("numVoices", _numVoices);
    
}

bool ChangeStepSeqMaxVoicesProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = imgui_util::InputText<128>("entityName", &_entityName);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("relative", &_relative);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputInt("numVoices", &_numVoices);
        changed = changed || thisChanged;
    }
    
    return changed;
}