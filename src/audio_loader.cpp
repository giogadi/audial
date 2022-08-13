#include "audio_loader.h"

#include <fstream>
#include <iostream>

#include "boost/property_tree/xml_parser.hpp"

bool SaveSynthPatches(char const* filename, std::vector<synth::Patch> const& synthPatches) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "Failed to open patch filename \"" << filename << "\" for saving." << std::endl;
        return false;
    }
    ptree patchesPt;
    for (synth::Patch const& patch : synthPatches) {
        serial::SaveInNewChildOf(patchesPt, "patch", patch);
    }
    ptree pt;
    pt.add_child("patches", patchesPt);
    boost::property_tree::xml_parser::xml_writer_settings<std::string> settings(' ', 4);
    boost::property_tree::write_xml(outFile, pt, settings);
    return true;
}

bool LoadSynthPatches(char const* filename, std::vector<synth::Patch>& patches) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Failed to open patch filename \"" << filename << "\" for loading." << std::endl;
        return false;
    }
    ptree pt;
    boost::property_tree::read_xml(inFile, pt);
    for (auto const& item : pt.get_child("patches")) {
        ptree const& patchPt = item.second;
        patches.emplace_back();
        synth::Patch& patch = patches.back();
        patch.Load(patchPt);
    }
    return true;
}