#include "src/properties/NoteOnOffSeqAction.h"

#include "imgui/imgui.h"

#include "imgui_util.h"



void NoteOnOffSeqActionProps::Load(serial::Ptree pt) {
   
   pt.TryGetInt("channel", &_channel);
   
   pt.TryGetInt("midiNote", &_midiNote);
   
   pt.TryGetDouble("noteLength", &_noteLength);
   
   pt.TryGetFloat("velocity", &_velocity);
   
   pt.TryGetDouble("quantizeDenom", &_quantizeDenom);
   
}

void NoteOnOffSeqActionProps::Save(serial::Ptree pt) const {
    
    pt.PutInt("channel", _channel);
    
    pt.PutInt("midiNote", _midiNote);
    
    pt.PutDouble("noteLength", _noteLength);
    
    pt.PutFloat("velocity", _velocity);
    
    pt.PutDouble("quantizeDenom", _quantizeDenom);
    
}

bool NoteOnOffSeqActionProps::ImGui() {
    bool changed = false;
    
    {
        bool thisChanged = ImGui::InputInt("channel", &_channel);
        changed = changed || thisChanged;
    }
    
    {
        bool thisChanged = ImGui::InputInt("midiNote", &_midiNote);
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
    
    return changed;
}