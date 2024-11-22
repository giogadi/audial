#pragma once

#include <synth.h>
#include <audio_platform.h>
#include <vector>
#include "synth_patch_bank.h"

struct SynthGuiState {
    synth::PatchBank* _synthPatches = nullptr;  // not owned
    std::string _saveFilename;
    int _currentPatchBankIx = 0;
    int _currentSynthIx = -1;
    synth::Patch _currentPatch;
};

void DrawSynthGuiAndUpdatePatch(SynthGuiState& synthGuiState,
				audio::Context& audioContext);

