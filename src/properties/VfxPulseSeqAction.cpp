#include "src/properties/VfxPulseSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"



void VfxPulseSeqActionProps::Load(serial::Ptree pt) {
   
   pt.TryGetString("vfxEntityName", &_vfxEntityName);
   
}

void VfxPulseSeqActionProps::Save(serial::Ptree pt) const {
    
    pt.PutString("vfxEntityName", _vfxEntityName.c_str());
    
}

bool VfxPulseSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = imgui_util::InputText<128>("vfxEntityName", &_vfxEntityName);
        changed = changed || thisChanged;
    }
    
    return changed;
}