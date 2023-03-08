#pragma once

#include "transform.h"
#include "game_manager.h"
#include "new_entity.h"

namespace geometry {

bool SegmentBoxIntersection2d(
    float sx0, float sy0, float sx1, float sy1, float minBoxX, float minBoxY, float maxBoxX, float maxBoxY);

bool IsSegmentCollisionFree2D(GameManager& g, Vec3 const& playerPos, Vec3 const& enemyPos, ne::EntityType entityTypeToCheck);

// Ignores rotations of transforms.
// penetration can be nullptr.
// NOTE: penetration points from 2 to 1.
// NOTE: penetration only has a value if this returns TRUE.
bool DoAABBsOverlap(Transform const& aabb1, Transform const& aabb2, Vec3* penetration);

}  // namespace geometry
