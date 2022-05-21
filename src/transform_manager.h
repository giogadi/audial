#pragma once

#include <optional>

#include "component.h"

class TransformManager {
public:
    void AddTransform(TransformComponent* t, Entity* e) {
        _transforms.emplace_back(t, e);
    }
    void RemoveTransform(TransformComponent const* t);

    std::optional<std::pair<TransformComponent*, Entity*>> PickEntity(
        double clickX, double clickY, int windowWidth, int windowHeight,
        float fovy, float aspectRatio, float zNear, TransformComponent const& cameraTransform);

    std::vector<std::pair<TransformComponent*, Entity*>> _transforms;
};