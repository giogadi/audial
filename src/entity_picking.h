#pragma once

#include <memory>

#include "entity_manager.h"
#include "new_entity.h"
#include "matrix.h"

namespace renderer {
    class Camera;
}

// screenX/screenY go from [0,width/height]
// screenY is 0 at TOP of screen.
void GetPickRay(
    double screenX, double screenY, int windowWidth, int windowHeight, renderer::Camera const& camera,
    Vec3* rayStart, Vec3* rayDir);

EntityId PickEntity(
    EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
    renderer::Camera const& camera);

ne::Entity* PickEntity(
    ne::EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
    renderer::Camera const& camera);
