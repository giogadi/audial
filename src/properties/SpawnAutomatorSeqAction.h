#pragma once


#include "enums/audio_SynthParamType.h"


#include "serial.h"

struct SpawnAutomatorSeqActionProps {
    
    bool _relative = false;
    
    float _startValue = 0.f;
    
    float _endValue = 1.f;
    
    double _desiredAutomateTime = 1.0;
    
    bool _synth = false;
    
    audio::SynthParamType _synthParam = audio::SynthParamType::Gain;
    
    int _channel = 0;
    
    std::string _seqEntityName;
    

    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui();
};