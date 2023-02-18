#include "entity_picking.h"

#include <optional>

#include "renderer.h"

namespace {

std::optional<float> sphereRayCast(Vec3 const& rayStart, Vec3 const& normalizedRayDir, Vec3 const& spherePos, float const sphereRadius) {
    // a: ||rayDir||^2 == 1
    // b: dot(2*rayDir, rayStart - spherePos)
    // c: ||rayStart - spherePos||^2 - radius^2
    Vec3 sphereToRayStart = rayStart - spherePos;
    float a = 1.f;
    float b  = 2*(Vec3::Dot(normalizedRayDir, sphereToRayStart));
    float c = sphereToRayStart.Length2() - (sphereRadius*sphereRadius);

    float discriminant = b*b - 4*a*c;
    if (discriminant < 0.f) {
        return std::nullopt;
    }

    float rootDisc = sqrt(discriminant);
    float t1 = (-b + rootDisc) / (2*a);
    float t2 = (-b - rootDisc) / (2*a);

    // Return the t >= 0 that is lower in magnitude. If t2 is >= 0 it's guaranteed to be the closer hit.
    if (t2 >= 0) {
        return t2;
    } else if (t1 >= 0) {
        return t1;
    } else {
        return std::nullopt;
    }
}

}  // end namespace

void GetPickRay(double screenX, double screenY, int windowWidth, int windowHeight, renderer::Camera const& camera,
    Vec3* rayStart, Vec3* rayDir) {
    assert(rayStart != nullptr);
    assert(rayDir != nullptr);

    Mat4 const& cameraTransform = camera._transform;    

    float aspectRatio = (float) windowWidth / (float) windowHeight;

    if (camera._projectionType == renderer::Camera::ProjectionType::Perspective) {

        // First we find the world-space position of the top-left corner of the near clipping plane.
        Vec3 nearPlaneCenter = cameraTransform.GetPos() - cameraTransform.GetZAxis() * camera._zNear;
        float nearPlaneHalfHeight = camera._zNear * tan(0.5f * camera._fovyRad);
        float nearPlaneHalfWidth = nearPlaneHalfHeight * aspectRatio;
        Vec3 nearPlaneTopLeft = nearPlaneCenter;
        nearPlaneTopLeft -= nearPlaneHalfWidth * cameraTransform.GetXAxis();
        nearPlaneTopLeft += nearPlaneHalfHeight * cameraTransform.GetYAxis();

        // Now map clicked point from [0,windowWidth] -> [0,1]
        float xFactor = screenX / windowWidth;
        float yFactor = screenY / windowHeight;

        Vec3 clickedPointOnNearPlane = nearPlaneTopLeft;
        clickedPointOnNearPlane += (xFactor * 2 * nearPlaneHalfWidth) * cameraTransform.GetXAxis();
        clickedPointOnNearPlane -= (yFactor * 2 * nearPlaneHalfHeight) * cameraTransform.GetYAxis();

        *rayDir = (clickedPointOnNearPlane - cameraTransform.GetPos()).GetNormalized();
        *rayStart = cameraTransform.GetPos() ;
    } else if (camera._projectionType == renderer::Camera::ProjectionType::Orthographic) {
        // get screen units as [0,1]
        double xFactor = screenX / (double) windowWidth;
        // Flip it so that yFactor is 0 at screen bottom
        double yFactor = 1.0 - (screenY / (double) windowHeight);

        // map xFactor/yFactor to world units local to camera x/y
        float offsetFromCameraX = (xFactor - 0.5) * camera._width;
        float viewHeight = camera._width / aspectRatio;
        float offsetFromCameraY = (yFactor - 0.5) * viewHeight;

        Vec4 posRelToCamera(offsetFromCameraX, offsetFromCameraY, 0.f, 1.f);
        Vec4 worldPos = camera._transform * posRelToCamera;
        rayStart->Set(worldPos._x, worldPos._y, worldPos._z);
        *rayDir = -camera._transform.GetZAxis();        
    } else { assert(false); }
}

// https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_custom.cpp
bool TestRayOBBIntersection(
	Vec3 const& ray_origin,        // Ray origin, in world space
	Vec3 const& ray_direction,     // Ray direction (NOT target position!), in world space. Must be normalize()'d.
	Vec3 const& aabb_min,          // Minimum X,Y,Z coords of the mesh when not transformed at all.
	Vec3 const& aabb_max,          // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is centered, but it's not always the case.
	Mat4 const& ModelMatrix,       // Transformation applied to the mesh (which will thus be also applied to its bounding box)
	float& intersection_distance // Output : distance between ray_origin and the intersection with the OBB
){
	
	// Intersection method from Real-Time Rendering and Essential Mathematics for Games
	
	float tMin = 0.0f;
	float tMax = 100000.0f;

	// Vec3 OBBposition_worldspace(ModelMatrix[3].x, ModelMatrix[3].y, ModelMatrix[3].z);
    Vec3 OBBposition_worldspace = ModelMatrix.GetPos();

	Vec3 delta = OBBposition_worldspace - ray_origin;

	// Test intersection with the 2 planes perpendicular to the OBB's X axis
	{
		// glm::vec3 xaxis(ModelMatrix[0].x, ModelMatrix[0].y, ModelMatrix[0].z);
        Vec3 xaxis = ModelMatrix.GetCol3(0);
		float e = Vec3::Dot(xaxis, delta);
		float f = Vec3::Dot(ray_direction, xaxis);

		if ( fabs(f) > 0.001f ){ // Standard case

			float t1 = (e+aabb_min._x)/f; // Intersection with the "left" plane
			float t2 = (e+aabb_max._x)/f; // Intersection with the "right" plane
			// t1 and t2 now contain distances betwen ray origin and ray-plane intersections

			// We want t1 to represent the nearest intersection, 
			// so if it's not the case, invert t1 and t2
			if (t1>t2){
				float w=t1;t1=t2;t2=w; // swap t1 and t2
			}

			// tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
			if ( t2 < tMax )
				tMax = t2;
			// tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
			if ( t1 > tMin )
				tMin = t1;

			// And here's the trick :
			// If "far" is closer than "near", then there is NO intersection.
			// See the images in the tutorials for the visual explanation.
			if (tMax < tMin )
				return false;

		}else{ // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
			if(-e+aabb_min._x > 0.0f || -e+aabb_max._x < 0.0f)
				return false;
		}
	}


	// Test intersection with the 2 planes perpendicular to the OBB's Y axis
	// Exactly the same thing than above.
	{
		// glm::vec3 yaxis(ModelMatrix[1].x, ModelMatrix[1].y, ModelMatrix[1].z);
        Vec3 yaxis = ModelMatrix.GetCol3(1);
		float e = Vec3::Dot(yaxis, delta);
		float f = Vec3::Dot(ray_direction, yaxis);

		if ( fabs(f) > 0.001f ){

			float t1 = (e+aabb_min._y)/f;
			float t2 = (e+aabb_max._y)/f;

			if (t1>t2){float w=t1;t1=t2;t2=w;}

			if ( t2 < tMax )
				tMax = t2;
			if ( t1 > tMin )
				tMin = t1;
			if (tMin > tMax)
				return false;

		}else{
			if(-e+aabb_min._y > 0.0f || -e+aabb_max._y < 0.0f)
				return false;
		}
	}


	// Test intersection with the 2 planes perpendicular to the OBB's Z axis
	// Exactly the same thing than above.
	{
		// glm::vec3 zaxis(ModelMatrix[2].x, ModelMatrix[2].y, ModelMatrix[2].z);
        Vec3 zaxis = ModelMatrix.GetCol3(2);
		float e = Vec3::Dot(zaxis, delta);
		float f = Vec3::Dot(ray_direction, zaxis);

		if ( fabs(f) > 0.001f ){

			float t1 = (e+aabb_min._z)/f;
			float t2 = (e+aabb_max._z)/f;

			if (t1>t2){float w=t1;t1=t2;t2=w;}

			if ( t2 < tMax )
				tMax = t2;
			if ( t1 > tMin )
				tMin = t1;
			if (tMin > tMax)
				return false;

		}else{
			if(-e+aabb_min._z > 0.0f || -e+aabb_max._z < 0.0f)
				return false;
		}
	}

	intersection_distance = tMin;
	return true;

}

// EntityId PickEntity(
//     EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
//     renderer::Camera const& camera) {

//     float const pickSphereRad = 0.1f;

//     Vec3 rayStart;
//     Vec3 rayDir;
//     GetPickRay(clickX, clickY, windowWidth, windowHeight, camera, &rayStart, &rayDir);
//     // Start a little ahead of camera so we don't pick it
//     rayStart += -camera._transform.GetZAxis() * pickSphereRad * 1.1f;

//     std::optional<float> closestPickDist;
//     EntityId closestPickItem = EntityId::InvalidId();
//     entities.ForEveryActiveEntity([&](EntityId id) {
//         Entity& entity = *entities.GetEntity(id);
//         std::weak_ptr<TransformComponent> pTrans = entity.FindComponentOfType<TransformComponent>();
//         if (pTrans.expired()) {
//             return;
//         }
//         TransformComponent const& t = *pTrans.lock();
//         std::optional<float> hitDist = sphereRayCast(rayStart, rayDir, t.GetWorldPos(), pickSphereRad);
//         if (hitDist.has_value()) {
//             if (!closestPickDist.has_value() || *hitDist < *closestPickDist) {
//                 *closestPickDist = *hitDist;
//                 closestPickItem = id;
//             }
//         }
//     });

//     return closestPickItem;
// }

ne::Entity* PickEntity(
    ne::EntityManager& entityMgr, double clickX, double clickY, int windowWidth, int windowHeight,
    renderer::Camera const& camera) {

    Vec3 rayStart;
    Vec3 rayDir;
    GetPickRay(clickX, clickY, windowWidth, windowHeight, camera, &rayStart, &rayDir);

    std::optional<float> closestPickDist;
    ne::Entity* closestPickItem = nullptr;
    for (auto iter = entityMgr.GetAllIterator(); !iter.Finished(); iter.Next()) {
        ne::Entity& entity = *iter.GetEntity();
        if (!entity._pickable) {
            continue;
        }
        std::optional<float> hitDist;
        {
            // TODO: THIS DECOMPOSITION PROBABLY DOES NOT HANDLE SHEAR
            // Vec3 scale;
            
            // Vec3 xAxis = entity._transform.GetXAxis();
            // scale._x = xAxis.Normalize();

            // Vec3 yAxis = entity._transform.GetCol3(1);
            // scale._y = yAxis.Normalize();

            // Vec3 zAxis = entity._transform.GetCol3(2);
            // scale._z = zAxis.Normalize();

            // scale *= 0.5f;

            // Mat3 rot(xAxis, yAxis, zAxis);
            // Mat4 transNoScale;
            // transNoScale.SetTopLeftMat3(rot);
            // transNoScale.SetTranslation(pos);

            Vec3 scale = entity._transform.Scale();
            scale *= 0.5f;
            

            float d;
            if (TestRayOBBIntersection(rayStart, rayDir, -scale, scale, entity._transform.Mat4NoScale(), d)) {
                hitDist = d;
            }
        }
        if (hitDist.has_value()) {
            if (!closestPickDist.has_value() || *hitDist < *closestPickDist) {
                *closestPickDist = *hitDist;
                closestPickItem = &entity;
            }
        }       
    }

    return closestPickItem;
}
