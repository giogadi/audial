#include "src/properties/SpawnAutomatorSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"

#include "serial_enum.h"



void SpawnAutomatorSeqActionProps::Load(serial::Ptree pt) {
   
   pt.TryGetBool("relative", &_relative);
   
   pt.TryGetFloat("startValue", &_startValue);
   
   pt.TryGetFloat("endValue", &_endValue);
   
   pt.TryGetDouble("desiredAutomateTime", &_desiredAutomateTime);
   
   pt.TryGetBool("synth", &_synth);
   
   serial::TryGetEnum(pt, "synthParam", _synthParam);
   
   serial::TryGetEnum(pt, "stepSeqParam", _stepSeqParam);
   
   pt.TryGetInt("channel", &_channel);
   
   serial::LoadFromChildOf(pt, "seqEditorId", _seqEditorId);
   
}

void SpawnAutomatorSeqActionProps::Save(serial::Ptree pt) const {
    
    pt.PutBool("relative", _relative);
    
    pt.PutFloat("startValue", _startValue);
    
    pt.PutFloat("endValue", _endValue);
    
    pt.PutDouble("desiredAutomateTime", _desiredAutomateTime);
    
    pt.PutBool("synth", _synth);
    
    serial::PutEnum(pt, "synthParam", _synthParam);
    
    serial::PutEnum(pt, "stepSeqParam", _stepSeqParam);
    
    pt.PutInt("channel", _channel);
    
    serial::SaveInNewChildOf(pt, "seqEditorId", _seqEditorId);
    
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
        bool thisChanged = StepSeqParamTypeImGui("StepSeqParamType", &_stepSeqParam);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputInt("channel", &_channel);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputEditorId("seqEditorId", &_seqEditorId);
        changed = changed || thisChanged;
    }
    
    return changed;
}