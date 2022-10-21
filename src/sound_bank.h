#pragma once

#include <vector>

#include "audio_util.h"

class SoundBank {
public:
    void LoadSounds();
    void Destroy();
    std::vector<audio::PcmSound> _sounds;
    std::vector<char const*> _soundNames;
};