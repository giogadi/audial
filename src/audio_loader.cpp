#include "audio_loader.h"

#include <fstream>

#include "cereal/archives/xml.hpp"

#include "audio_serialize.h"

void LoadSynthPatch(char const* filename, synth::Patch& p) {
    std::ifstream inFile(filename);
    cereal::XMLInputArchive archive(inFile);
    archive(p);
}

void SaveSynthPatch(char const* filename, synth::Patch const& synthPatch) {
    std::ofstream outFile(filename);
    cereal::XMLOutputArchive archive(outFile);
    archive(CEREAL_NVP(synthPatch));
}