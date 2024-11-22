#pragma once

#include "audio.h"

namespace audio {
    struct Context {
        int _outputSampleRate = -1;        
        StateData _state;

        // returns true on success
        bool Init(SoundBank const &soundBank);

        void ShutDown();

        bool AddEvent(Event const &e);        

        double GetAudioTime();
    };
}