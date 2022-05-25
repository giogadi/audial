#pragma once

#include <vector>

#include "synth.h"

bool SaveSynthPatches(char const* filename, std::vector<synth::Patch> const& patches);
bool LoadSynthPatches(char const* filename, std::vector<synth::Patch>& patches);