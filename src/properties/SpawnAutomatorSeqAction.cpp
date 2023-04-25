#include "src/properties/SpawnAutomatorSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"



void SpawnAutomatorSeqActionProps::Load(serial::Ptree pt) {
   
   pt.TryGetBool("relative", &_relative);
   
   pt.TryGetFloat("startValue", &_startValue);
   
   pt.TryGetFloat("endValue", &_endValue);
   
   pt.TryGetDouble("desiredAutomateTime", &_desiredAutomateTime);
   
   pt.TryGetBool("synth", &_synth);
   
   serial::TryGetEnum(pt, "synthParam", _synthParam);
   
   pt.TryGetInt("channel", &_channel);
   
   pt.TryGetString("seqEntityName", &_seqEntityName);
   
}

void SpawnAutomatorSeqActionProps::Save(serial::Ptree pt) const {
    
    pt.PutBool("relative", _relative);
    
    pt.PutFloat("startValue", _startValue);
    
    pt.PutFloat("endValue", _endValue);
    
    pt.PutDouble("desiredAutomateTime", _desiredAutomateTime);
    
    pt.PutBool("synth", _synth);
    
    serial::PutEnum(pt, "synthParam", _synthParam);
    
    pt.PutInt("channel", _channel);
    
    pt.PutString("seqEntityName", _seqEntityName.c_str());
    
}

bool SpawnAutomatorSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = ImGui::Checkbox("relative", &_relative);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("startValue", &_startValue);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("endValue", &_endValue);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputDouble("desiredAutomateTime", &_desiredAutomateTime);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("synth", &_synth);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = audio::SynthParamTypeImGui("SynthParamType", &_synthParam);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputInt("channel", &_channel);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputText<128>("seqEntityName", &_seqEntityName);
        changed = changed || thisChanged;
    }
    
    return changed;
}