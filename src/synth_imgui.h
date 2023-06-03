#pragma once

#include <synth.h>
#include <audio.h>
#include <vector>
#include "synth_patch_bank.h"

struct SynthGuiState {
    synth::PatchBank _synthPatches;
    std::string _saveFilename;
    int _currentSynthIx = -1;
};

void DrawSynthGuiAndUpdatePatch(SynthGuiState& synthGuiState,
				audio::Context& audioContext);

