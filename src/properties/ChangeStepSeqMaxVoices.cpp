#include "src/properties/ChangeStepSeqMaxVoices.h"

#include "imgui/imgui.h"

#include "imgui_util.h"



void ChangeStepSeqMaxVoicesProps::Load(serial::Ptree pt) {
   
   serial::LoadFromChildOf(pt, "seqEditorId", _seqEditorId);
   
   pt.TryGetBool("relative", &_relative);
   
   pt.TryGetInt("numVoices", &_numVoices);
   
}

void ChangeStepSeqMaxVoicesProps::Save(serial::Ptree pt) const {
    
    serial::SaveInNewChildOf(pt, "seqEditorId", _seqEditorId);
    
    pt.PutBool("relative", _relative);
    
    pt.PutInt("numVoices", _numVoices);
    
}

bool ChangeStepSeqMaxVoicesProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = imgui_util::InputEditorId("seqEditorId", &_seqEditorId);
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