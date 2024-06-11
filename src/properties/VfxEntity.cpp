#include "src/properties/VfxEntity.h"

#include "imgui/imgui.h"

#include "imgui_util.h"

#include "serial_enum.h"



void VfxEntityProps::Load(serial::Ptree pt) {
   
   pt.TryGetFloat("cycleTimeSecs", &_cycleTimeSecs);
   
   pt.TryGetFloat("pulseScaleFactor", &_pulseScaleFactor);
   
}

void VfxEntityProps::Save(serial::Ptree pt) const {
    
    pt.PutFloat("cycleTimeSecs", _cycleTimeSecs);
    
    pt.PutFloat("pulseScaleFactor", _pulseScaleFactor);
    
}

bool VfxEntityProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = ImGui::InputFloat("cycleTimeSecs", &_cycleTimeSecs);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("pulseScaleFactor", &_pulseScaleFactor);
        changed = changed || thisChanged;
    }
    
    return changed;
}