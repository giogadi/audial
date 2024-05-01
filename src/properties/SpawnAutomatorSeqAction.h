#pragma once


#include "enums/StepSeqParamType.h"

#include "enums/audio_SynthParamType.h"

#include "editor_id.h"


#include "serial.h"

struct SpawnAutomatorSeqActionProps {
    
    bool _relative = false;
    
    float _startValue = 0.f;
    
    float _endValue = 1.f;
    
    double _desiredAutomateTime = 1.0;
    
    bool _synth = false;
    
    audio::SynthParamType _synthParam = audio::SynthParamType::Gain;
    
    StepSeqParamType _stepSeqParam = StepSeqParamType::Velocities;
    
    int _channel = 0;
    
    EditorId _seqEditorId;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};