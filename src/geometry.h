#pragma once

#include <vector>

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

// Assume convexPoly is convex and points are in CCW order. Ignores y-coords of inputs. penetration points OUT of polygon.
bool PointInConvexPolygon2D(Vec3 const& queryP, std::vector<Vec3> const& convexPoly, Vec3 const& polyPos, float polyRotRad, Vec3 const& polyScale, Vec3* penetration);

// General polygons without holes
bool PointInPolygon2D(Vec3 const& queryP, Vec3 const* polyPoints, int polyPointCount);

void ProjectWorldPointToScreenSpace(Vec3 const& worldPos, Mat4 const& viewProjMatrix, int screenWidth, int screenHeight, float& screenX, float& screenY);

bool IsPointInBoundingBox(Vec3 const& p, Transform const& transform);
bool IsPointInBoundingBox2d(Vec3 const& p, Transform const& transform);

}  // namespace geometry
