#include "src/properties/SetAllStepsSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"


#include "serial_vector_util.h"

#include "imgui_vector_util.h"


void SetAllStepsSeqActionProps::Load(serial::Ptree pt) {
   
   serial::LoadFromChildOf(pt, "seqEntityEditorId", _seqEntityEditorId);
   
   pt.TryGetBool("velOnly", &_velOnly);
   
   serial::LoadVectorFromChildNode<MidiNoteAndName>(pt, "midiNotes", _midiNotes);
   
   pt.TryGetFloat("velocity", &_velocity);
   
   pt.TryGetInt("trackIx", &_trackIx);
   
}

void SetAllStepsSeqActionProps::Save(serial::Ptree pt) const {
    
    serial::SaveInNewChildOf(pt, "seqEntityEditorId", _seqEntityEditorId);
    
    pt.PutBool("velOnly", _velOnly);
    
    serial::SaveVectorInChildNode<MidiNoteAndName>(pt, "midiNotes", "array_item", _midiNotes);
    
    pt.PutFloat("velocity", _velocity);
    
    pt.PutInt("trackIx", _trackIx);
    
}

bool SetAllStepsSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        
        bool thisChanged = imgui_util::InputEditorId("seqEntityEditorId", &_seqEntityEditorId);
        changed = changed || thisChanged;
        
    }
    
    {
        
        bool thisChanged = ImGui::Checkbox("velOnly", &_velOnly);
        changed = changed || thisChanged;
        
    }
    
    {
        
        imgui_util::InputVectorOptions options;
        
        options.removeOnSameLine = true;
        
        if (ImGui::TreeNode("midiNotes")) {
        
        bool thisChanged = imgui_util::InputVector(_midiNotes, options);
        changed = changed || thisChanged;
        
        ImGui::TreePop();
        
        }
        
    }
    
    {
        
        bool thisChanged = ImGui::InputFloat("velocity", &_velocity);
        changed = changed || thisChanged;
        
    }
    
    {
        
        bool thisChanged = ImGui::InputInt("trackIx", &_trackIx);
        changed = changed || thisChanged;
        
    }
    
    return changed;
}