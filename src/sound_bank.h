#pragma once

#include <vector>

#include "audio_util.h"

class SoundBank {
public:
    void LoadSounds(int sampleRate);
    void Destroy();    
    int GetSoundIx(char const* soundName) const;  // returns -1 if not found
    std::vector<audio::PcmSound> _sounds;
    std::vector<char const*> _soundNames;
    std::vector<int> _exclusiveGroups;
};
