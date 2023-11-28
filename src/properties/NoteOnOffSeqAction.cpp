#include "src/properties/NoteOnOffSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"


#include "serial_vector_util.h"

#include "imgui_vector_util.h"


void NoteOnOffSeqActionProps::Load(serial::Ptree pt) {
   
   pt.TryGetInt("channel", &_channel);
   
   serial::LoadVectorFromChildNode<MidiNoteAndName>(pt, "midiNotes", _midiNotes);
   
   pt.TryGetDouble("noteLength", &_noteLength);
   
   pt.TryGetFloat("velocity", &_velocity);
   
   pt.TryGetDouble("quantizeDenom", &_quantizeDenom);
   
   pt.TryGetBool("doNoteOff", &_doNoteOff);
   
   pt.TryGetBool("holdNotes", &_holdNotes);
   
}

void NoteOnOffSeqActionProps::Save(serial::Ptree pt) const {
    
    pt.PutInt("channel", _channel);
    
    serial::SaveVectorInChildNode<MidiNoteAndName>(pt, "midiNotes", "array_item", _midiNotes);
    
    pt.PutDouble("noteLength", _noteLength);
    
    pt.PutFloat("velocity", _velocity);
    
    pt.PutDouble("quantizeDenom", _quantizeDenom);
    
    pt.PutBool("doNoteOff", _doNoteOff);
    
    pt.PutBool("holdNotes", _holdNotes);
    
}

bool NoteOnOffSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = ImGui::InputInt("channel", &_channel);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = imgui_util::InputVector(_midiNotes);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputDouble("noteLength", &_noteLength);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputFloat("velocity", &_velocity);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputDouble("quantizeDenom", &_quantizeDenom);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("doNoteOff", &_doNoteOff);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::Checkbox("holdNotes", &_holdNotes);
        changed = changed || thisChanged;
    }
    
    return changed;
}