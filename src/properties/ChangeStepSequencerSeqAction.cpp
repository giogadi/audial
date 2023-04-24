#include "src/properties/ChangeStepSequencerSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"



void ChangeStepSequencerSeqActionProps::Load(serial::Ptree pt) {
   
   pt.TryGetString("seqName", &_seqName);
   
   pt.TryGetBool("velOnly", &_velOnly);
   
   pt.TryGetBool("temporary", &_temporary);
   
   pt.TryGetString("stepStr", &_stepStr);
   
}

void ChangeStepSequencerSeqActionProps::Save(serial::Ptree pt) const {
    
    pt.PutString("seqName", _seqName.c_str());
    
    pt.PutBool("velOnly", _velOnly);
    
    pt.PutBool("temporary", _temporary);
    
    pt.PutString("stepStr", _stepStr.c_str());
    
}

bool ChangeStepSequencerSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = imgui_util::InputText<128>("seqName", &_seqName);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("velOnly", &_velOnly);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("temporary", &_temporary);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputText<128>("stepStr", &_stepStr);
        changed = changed || thisChanged;
    }
    
    return changed;
}