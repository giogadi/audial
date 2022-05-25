#include "audio_loader.h"

#include <fstream>

#include "cereal/archives/xml.hpp"
#include "cereal/types/vector.hpp"

#include "audio_serialize.h"

bool SaveSynthPatches(char const* filename, std::vector<synth::Patch> const& synthPatches) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "Failed to open patch filename \"" << filename << "\" for saving." << std::endl;
        return false;
    }
    cereal::XMLOutputArchive archive(outFile);
    archive(CEREAL_NVP(synthPatches));
    return true;
}

bool LoadSynthPatches(char const* filename, std::vector<synth::Patch>& patches) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Failed to open patch filename \"" << filename << "\" for loading." << std::endl;
        return false;
    }
    cereal::XMLInputArchive archive(inFile);
    archive(patches);
    return true;
}