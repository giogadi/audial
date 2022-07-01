#pragma once

#include <memory>

#include "component.h"

class TransformComponent;

EntityId PickEntity(
    EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
    float fovy, float aspectRatio, float zNear, TransformComponent const& cameraTransform);