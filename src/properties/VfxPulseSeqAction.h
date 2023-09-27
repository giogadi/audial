#pragma once


#include "editor_id.h"


#include "serial.h"

struct VfxPulseSeqActionProps {
    
    EditorId _vfxEditorId;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};