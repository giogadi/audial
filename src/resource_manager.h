#pragma once

#include <unordered_map>
#include <memory>

class BoundMesh;

struct ModelManager {
    std::unordered_map<std::string, std::unique_ptr<BoundMesh>> _modelMap;
};