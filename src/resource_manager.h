#pragma once

#include <unordered_map>
#include <memory>

class BoundMeshPNU;

struct MeshManager {
    std::unordered_map<std::string, std::unique_ptr<BoundMeshPNU>> _meshMap;
};