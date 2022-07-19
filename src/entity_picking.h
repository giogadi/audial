#pragma once

#include <memory>

#include "entity_manager.h"
#include "matrix.h"

namespace renderer {
    class Camera;
}

EntityId PickEntity(
    EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
    renderer::Camera const& camera);