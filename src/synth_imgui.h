#pragma once

#include <synth.h>
#include <audio.h>

struct SynthGuiState {
    std::vector<synth::Patch> _synthPatches;
    std::string _saveFilename;
    int _currentSynthIx = -1;
};


void InitSynthGuiState(audio::Context const& audioContext, char const* saveFilename,
		       SynthGuiState& guiState);

void DrawSynthGuiAndUpdatePatch(SynthGuiState& synthGuiState,
				audio::Context& audioContext);

