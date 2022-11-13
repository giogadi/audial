#include "audio_loader.h"

bool SaveSynthPatches(char const* filename, std::vector<synth::Patch> const& synthPatches) {
    serial::Ptree pt = serial::Ptree::MakeNew();
    serial::Ptree patchesPt = pt.AddChild("patches");
    for (synth::Patch const& patch : synthPatches) {
        serial::Ptree patchPt = patchesPt.AddChild("patch");
        patch.Save(patchPt);
    }
    bool success = pt.WriteToFile(filename);
    if (!success) {
        printf("Failed to save patches to filename \"%s\".\n", filename);
        return false;
    }
    return true;
}

bool LoadSynthPatches(char const* filename, std::vector<synth::Patch>& patches) {
    serial::Ptree pt = serial::Ptree::MakeNew();
    if (!pt.LoadFromFile(filename)) {
        printf("Failed to load patches from file \"%s\".\n", filename);
        return false;
    }
    serial::Ptree patchesPt = pt.GetChild("patches");
    int numPatches = 0;
    serial::NameTreePair* children = patchesPt.GetChildren(&numPatches);
    for (int i = 0; i < numPatches; ++i) {
        patches.emplace_back();
        synth::Patch& patch = patches.back();
        patch.Load(children[i]._pt);
    }
    delete[] children;
    return true;
}
