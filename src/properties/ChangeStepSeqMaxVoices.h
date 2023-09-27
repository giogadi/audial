#pragma once


#include "editor_id.h"


#include "serial.h"

struct ChangeStepSeqMaxVoicesProps {
    
    EditorId _seqEditorId;
    
    bool _relative = false;
    
    int _numVoices = 0;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};