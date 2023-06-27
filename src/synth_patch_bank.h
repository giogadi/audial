#pragma once

#include <vector>
#include <string>

#include "synth_patch.h"

namespace synth {

struct PatchBank {
    void Clear() {
        _names.clear();
        _patches.clear();
    }
    Patch* GetPatch(std::string_view name);
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    std::vector<std::string> _names;
    std::vector<Patch> _patches;
};

}  // namespace synth
