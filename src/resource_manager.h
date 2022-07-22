#pragma once

#include <unordered_map>
#include <memory>
// TODO: for some reason we need to include this for _meshMap. I don't get it. Maybe do a map of raw pointers.
#include "mesh.h"

class BoundMeshPNU;

struct MeshManager {
    std::unordered_map<std::string, std::unique_ptr<BoundMeshPNU>> _meshMap;
};