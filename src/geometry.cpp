#include "geometry.h"

namespace geometry {

float constexpr kEps = 0.0001f;

bool SegmentBoxIntersection2d(
    float sx0, float sy0, float sx1, float sy1, float minBoxX, float minBoxY, float maxBoxX, float maxBoxY) {
    float dsx = sx1 - sx0;
    float dsy = sy1 - sy0;
    // Left side of box
    {
        float col_t;
        if (std::abs(dsx) < kEps) {
            if (sx0 >= minBoxX && sx0 <= maxBoxX) {
                col_t = 0.f;
            } else {
                col_t = -1.f;
            }
        } else {
            col_t = (minBoxX - sx0) / dsx;
        }
        if (col_t >= 0.f && col_t <= 1.f) {
            float col_y = sy0 + col_t * dsy;
            if (col_y >= minBoxY && col_y <= maxBoxY) {
                return true;
            }
        }
    }

    // right side of box
    {
        float col_t;
        if (std::abs(dsx) < kEps) {
            if (sx0 >= minBoxX && sx0 <= maxBoxX) {
                col_t = 0.f;
            } else {
                col_t = -1.f;
            }
        } else {
            col_t = (maxBoxX - sx0) / dsx;
        }
        if (col_t >= 0.f && col_t <= 1.f) {
            float col_y = sy0 + col_t * dsy;
            if (col_y >= minBoxY && col_y <= maxBoxY) {
                return true;
            }
        }
    }

    // bottom side of box
    {
        float col_t;
        if (std::abs(dsy) < kEps) {
            if (sy0 >= minBoxY && sy0 <= maxBoxY) {
                col_t = 0.f;
            } else {
                col_t = -1.f;
            }
        } else {
            col_t = (minBoxY - sy0) / dsy;
        }
        if (col_t >= 0.f && col_t <= 1.f) {
            float col_x = sx0 + col_t * dsx;
            if (col_x >= minBoxX && col_x <= maxBoxX) {
                return true;
            }
        }
    }

    // top side of box
    {
        float col_t;
        if (std::abs(dsy) < kEps) {
            if (sy0 >= minBoxY && sy0 <= maxBoxY) {
                col_t = 0.f;
            } else {
                col_t = -1.f;
            }
        } else {
            col_t = (maxBoxY - sy0) / dsy;
        }
        if (col_t >= 0.f && col_t <= 1.f) {
            float col_x = sx0 + col_t * dsx;
            if (col_x >= minBoxX && col_x <= maxBoxX) {
                return true;
            }
        }
    }
    
    return false;
}

bool IsSegmentCollisionFree2D(GameManager& g, Vec3 const& playerPos, Vec3 const& enemyPos, ne::EntityType entityTypeToCheck) {
    int numEntities = 0;
    ne::EntityManager::Iterator wallIter = g._neEntityManager->GetIterator(entityTypeToCheck, &numEntities);
    for (; !wallIter.Finished(); wallIter.Next()) {
        // TODO: handle rotated walls. For now we assume AABB and ignore rotation.
        Transform const& wallTrans = wallIter.GetEntity()->_transform;
        Vec3 const& wallPos = wallTrans.Pos();
        Vec3 const& scale = wallTrans.Scale();
        float wallMinX = wallPos._x - 0.5f*scale._x;
        float wallMaxX = wallPos._x + 0.5f*scale._x;
        float wallMinZ = wallPos._z - 0.5f*scale._z;
        float wallMaxZ = wallPos._z + 0.5f*scale._z;
        bool hit = SegmentBoxIntersection2d(
            playerPos._x, playerPos._z, enemyPos._x, enemyPos._z,
            wallMinX, wallMinZ, wallMaxX, wallMaxZ);
        if (hit) {
            return false;
        }
    }
    return true;
}

// Checks XZ collisions between
// NOTE: penetration points from 2 to 1.
// NOTE: penetration only has a value if this returns TRUE.
bool DoAABBsOverlap(Transform const& aabb1, Transform const& aabb2, Vec3* penetration) {
    Vec3 const& p1 = aabb1.Pos();
    Vec3 const& p2 = aabb2.Pos();    
    Vec3 halfScale1 = 0.5f * aabb1.Scale();
    Vec3 halfScale2 = 0.5f * aabb2.Scale();
    Vec3 min1 = p1 - halfScale1;
    Vec3 min2 = p2 - halfScale2;
    Vec3 max1 = p1 + halfScale1;
    Vec3 max2 = p2 + halfScale2;
    // negative is overlap
    float maxSignedDist = std::numeric_limits<float>::lowest();
    int maxSignedDistAxis = -1;
    for (int i = 0; i < 3; ++i) {
        float dist1 = min1(i) - max2(i);
        if (dist1 > 0.f) {
            return false;
        }
        float dist2 = min2(i) - max1(i);
        if (dist2 > 0.f) {
            return false;
        }
        float maxDist = std::max(dist1, dist2);
        if (maxDist > maxSignedDist) {
            maxSignedDistAxis = i;
            maxSignedDist = maxDist;
        }
    }
    if (penetration) {
        assert(maxSignedDistAxis >= 0);
        assert(maxSignedDist <= 0);
        float sign = p1(maxSignedDistAxis) >= p2(maxSignedDistAxis) ? -1.f : 1.f;
        (*penetration)(maxSignedDistAxis) = sign * maxSignedDist;
    }
    return true;
}

bool PointInConvexPolygon2D(Vec3 const& queryP, std::vector<Vec3> const& convexPoly, Vec3 const& polyPos, float polyRotRad, Vec3 const& polyScale, Vec3* penetration) {
    Vec3 queryPTemp = queryP;
    queryPTemp -= polyPos;
    float c = cos(-polyRotRad);
    float s = sin(-polyRotRad);
    Vec3 queryPLocal;
    queryPLocal._x = c * queryPTemp._x - s * queryPTemp._z;
    queryPLocal._z = s * queryPTemp._x + c * queryPTemp._z;
    queryPLocal._x /= polyScale._x;
    queryPLocal._z /= polyScale._z;        
    float minPenetrationDistSqr = 99999.f;
    Vec3 minPenetrationNormal;  // will point OUT of polygon.
    for (int i = 0, n = convexPoly.size(); i < n; ++i) {
        Vec3 p1 = convexPoly[i];
        Vec3 p2 = convexPoly[(i+1) % n];

        Vec3 u = p2 - p1;
        u._y = 0.f;
        u.Normalize();
        Vec3 v = queryPLocal - p1;
        v._y = 0.f;
        Vec3 cross = Vec3::Cross(u, v);
        if (cross._y < 0.f) {
            return false;
        }
        // Get penetration vec
        float along = Vec3::Dot(v, u);
        Vec3 proj = along * u;
        Vec3 ortho = v - proj;
        float depth2 = ortho.Length2();
        if (depth2 < minPenetrationDistSqr) {
            minPenetrationDistSqr = depth2;
            minPenetrationNormal = -ortho;
        }
    }

    // Finally, transform the minPenetrationNormal to world coordinates
    s = -s;
    Vec3 normalRotated;
    normalRotated._x = c * minPenetrationNormal._x - s * minPenetrationNormal._z;
    normalRotated._z = s * minPenetrationNormal._x + c * minPenetrationNormal._z;
    if (penetration != nullptr) {
        *penetration = normalRotated;
    }
    
    return true;
}

void ProjectWorldPointToScreenSpace(Vec3 const& worldPos, Mat4 const& viewProjMatrix, int screenWidth, int screenHeight, float& screenX, float& screenY) {
    Vec4 pos(worldPos._x, worldPos._y, worldPos._z, 1.f);
    pos = viewProjMatrix * pos;
    pos.Set(pos._x / pos._w, pos._y / pos._w, pos._z / pos._w, 1.f);
    // map [-1,1] x [-1,1] to [0,sw] x [0,sh]
    screenX = (pos._x * 0.5f + 0.5f) * screenWidth;
    screenY = (-pos._y * 0.5f + 0.5f) * screenHeight;
}

bool IsPointInBoundingBox(Vec3 const& p, Transform const& transform) {
    Vec3 dpGlobal = p - transform.GetPos();
    Quaternion localToGlobal = transform.Quat();
    Quaternion globalToLocal = localToGlobal.Inverse();
    Mat3 globalToLocalMat;
    globalToLocal.GetRotMat(globalToLocalMat);
    Vec3 dpLocal = globalToLocalMat * dpGlobal;
    Vec3 halfExtents = transform.Scale();
    halfExtents *= 0.5f;
    bool outside = dpLocal._x < -halfExtents._x || dpLocal._x > halfExtents._x || dpLocal._y < -halfExtents._y || dpLocal._y > halfExtents._y || dpLocal._z < -halfExtents._z || dpLocal._z > halfExtents._z;
    return !outside;
}

bool IsPointInBoundingBox2d(Vec3 const& p, Transform const& transform) {
    Vec3 dpGlobal = p - transform.GetPos();
    Quaternion localToGlobal = transform.Quat();
    Quaternion globalToLocal = localToGlobal.Inverse();
    Mat3 globalToLocalMat;
    globalToLocal.GetRotMat(globalToLocalMat);
    Vec3 dpLocal = globalToLocalMat * dpGlobal;
    Vec3 halfExtents = transform.Scale();
    halfExtents *= 0.5f;
    bool outside = dpLocal._x < -halfExtents._x || dpLocal._x > halfExtents._x || dpLocal._z < -halfExtents._z || dpLocal._z > halfExtents._z;
    return !outside;
}

}  // namespace geometry
