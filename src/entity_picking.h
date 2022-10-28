#pragma once

#include <memory>

#include "entity_manager.h"
#include "new_entity.h"
#include "matrix.h"

namespace renderer {
    class Camera;
}

void GetPickRay(
    double screenX, double screenY, int windowWidth, int windowHeight, renderer::Camera const& camera,
    Vec3* rayStart, Vec3* rayDir);

EntityId PickEntity(
    EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
    renderer::Camera const& camera);

ne::Entity* PickEntity(
    ne::EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
    renderer::Camera const& camera);