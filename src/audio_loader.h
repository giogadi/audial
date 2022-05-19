#pragma once

namespace synth {
    class Patch;
}

void LoadSynthPatch(char const* filename, synth::Patch& p);
void SaveSynthPatch(char const* filename, synth::Patch const& p);