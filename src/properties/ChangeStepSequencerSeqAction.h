#pragma once


#include "editor_id.h"


#include "serial.h"

struct ChangeStepSequencerSeqActionProps {
    
    EditorId _seqEntityEditorId;
    
    bool _velOnly;
    
    bool _temporary;
    
    std::string _stepStr;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};