#include "synth_patch_bank.h"

namespace synth {

void PatchBank::Save(serial::Ptree pt) const {
    assert(_names.size() == _patches.size());
    for (int ii = 0, n = _names.size(); ii < n; ++ii) {
        serial::Ptree child = pt.AddChild("patch");
        child.PutString("name", _names[ii].c_str());
        serial::SaveInNewChildOf(child, "patch", _patches[ii]);
    }
}

void PatchBank::Load(serial::Ptree pt) {
    _names.clear();
    _patches.clear();
    int numChildren = 0;
    serial::NameTreePair* children = pt.GetChildren(&numChildren);
    _names.reserve(numChildren);
    _patches.reserve(numChildren);
    for (int ii = 0; ii < numChildren; ++ii) {
        serial::NameTreePair* pair = &children[ii];
        int version;
        if (pair->_pt.TryGetInt("version", &version)) {
            // old-style patch.
            _names.push_back(pair->_pt.GetString("name"));
            synth::Patch& patch = _patches.emplace_back();
            patch.Load(pair->_pt);            
        } else {
            _names.push_back(pair->_pt.GetString("name"));
            serial::Ptree patchPt = pair->_pt.GetChild("patch");
            synth::Patch& patch = _patches.emplace_back();
            patch.Load(patchPt);
        }
    }
    delete[] children;
}

}  // namespace synth
