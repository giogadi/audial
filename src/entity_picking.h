#pragma once

#include <optional>

#include "new_entity.h"
#include "matrix.h"

namespace renderer {
    class Camera;
}

std::optional<float> SphereRayCast(Vec3 const& rayStart, Vec3 const& normalizedRayDir, Vec3 const& spherePos, float const sphereRadius);

// screenX/screenY go from [0,width/height]
// screenY is 0 at TOP of screen.
void GetPickRay(
    double screenX, double screenY, int windowWidth, int windowHeight, renderer::Camera const& camera,
    Vec3* rayStart, Vec3* rayDir);

ne::Entity* PickEntity(
    ne::EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
    renderer::Camera const& camera);

std::optional<float> EntityRaycast(ne::Entity const& entity, Vec3 const& rayStart, Vec3 const& rayDir);

std::optional<float> PickEntity(ne::Entity const& entity, double clickX, double clickY, int windowWidth, int windowHeight, renderer::Camera const& camera);

// Returns picked entities sorted by increasing dist from camera. does NOT clear
// entityDistPairs!
void PickEntities(ne::EntityManager& entityMgr, double clickX, double clickY, int windowWidth, int windowHeight, renderer::Camera const& camera, std::vector<std::pair<ne::Entity*, float>>& entityDistPairs);
