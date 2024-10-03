#include "src/properties/ChangeStepSequencerSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"

#include "serial_enum.h"


#include "imgui_vector_util.h"

#include "serial_vector_util.h"


void ChangeStepSequencerSeqActionProps::Load(serial::Ptree pt) {
   
   serial::LoadFromChildOf(pt, "seqEntityEditorId", _seqEntityEditorId);
   
   pt.TryGetBool("changeVel", &_changeVel);
   
   pt.TryGetBool("changeNote", &_changeNote);
   
   pt.TryGetBool("temporary", &_temporary);
   
   serial::LoadVectorFromChildNode<MidiNoteAndName>(pt, "midiNotes", _midiNotes);
   
   pt.TryGetFloat("velocity", &_velocity);
   
   serial::LoadVectorFromChildNode<SynthParamValue>(pt, "params", _params);
   
}

void ChangeStepSequencerSeqActionProps::Save(serial::Ptree pt) const {
    
    serial::SaveInNewChildOf(pt, "seqEntityEditorId", _seqEntityEditorId);
    
    pt.PutBool("changeVel", _changeVel);
    
    pt.PutBool("changeNote", _changeNote);
    
    pt.PutBool("temporary", _temporary);
    
    serial::SaveVectorInChildNode<MidiNoteAndName>(pt, "midiNotes", "array_item", _midiNotes);
    
    pt.PutFloat("velocity", _velocity);
    
    serial::SaveVectorInChildNode<SynthParamValue>(pt, "params", "array_item", _params);
    
}

bool ChangeStepSequencerSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        
        bool thisChanged = imgui_util::InputEditorId("seqEntityEditorId", &_seqEntityEditorId);
        changed = changed || thisChanged;
        
    }
    
    {
        
        bool thisChanged = ImGui::Checkbox("changeVel", &_changeVel);
        changed = changed || thisChanged;
        
    }
    
    {
        
        bool thisChanged = ImGui::Checkbox("changeNote", &_changeNote);
        changed = changed || thisChanged;
        
    }
    
    {
        
        bool thisChanged = ImGui::Checkbox("temporary", &_temporary);
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
        
        imgui_util::InputVectorOptions options;
        
        if (ImGui::TreeNode("params")) {
        
        bool thisChanged = imgui_util::InputVector(_params, options);
        changed = changed || thisChanged;
        
        ImGui::TreePop();
        
        }
        
    }
    
    return changed;
}