#include "src/properties/VfxPulseSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"

#include "serial_enum.h"



void VfxPulseSeqActionProps::Load(serial::Ptree pt) {
   
   serial::LoadFromChildOf(pt, "vfxEditorId", _vfxEditorId);
   
}

void VfxPulseSeqActionProps::Save(serial::Ptree pt) const {
    
    serial::SaveInNewChildOf(pt, "vfxEditorId", _vfxEditorId);
    
}

bool VfxPulseSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = imgui_util::InputEditorId("vfxEditorId", &_vfxEditorId);
        changed = changed || thisChanged;
    }
    
    return changed;
}