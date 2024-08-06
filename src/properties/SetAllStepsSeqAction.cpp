#include "src/properties/SetAllStepsSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"

#include "serial_enum.h"


#include "serial_vector_util.h"

#include "imgui_vector_util.h"


void SetAllStepsSeqActionProps::Load(serial::Ptree pt) {
   
   serial::LoadFromChildOf(pt, "seqEntityEditorId", _seqEntityEditorId);
   
   pt.TryGetBool("velOnly", &_velOnly);
   
   serial::LoadVectorFromChildNode<MidiNoteAndName>(pt, "midiNotes", _midiNotes);
   
   pt.TryGetFloat("velocity", &_velocity);
   
}

void SetAllStepsSeqActionProps::Save(serial::Ptree pt) const {
    
    serial::SaveInNewChildOf(pt, "seqEntityEditorId", _seqEntityEditorId);
    
    pt.PutBool("velOnly", _velOnly);
    
    serial::SaveVectorInChildNode<MidiNoteAndName>(pt, "midiNotes", "array_item", _midiNotes);
    
    pt.PutFloat("velocity", _velocity);
    
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
        
        bool thisChanged = imgui_util::InputVector(_midiNotes, options);
        changed = changed || thisChanged;
    }
    
    {
        
        bool thisChanged = ImGui::InputFloat("velocity", &_velocity);
        changed = changed || thisChanged;
    }
    
    return changed;
}