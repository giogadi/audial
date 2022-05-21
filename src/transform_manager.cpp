#include "transform_manager.h"

#include <optional>

#include "renderer.h"
#include "input_manager.h"

void TransformManager::RemoveTransform(TransformComponent const* t) {
    // TODO!!!!!!! THIS DOESN'T WORK. PLEASE DON'T ALLOW DELETING ENTITIES WITHOUT FIXING THIS
    // if (_selected.has_value() && _selected->first == t) {
    //     _selected.reset();
    // }
    _transforms.erase(std::remove_if(_transforms.begin(), _transforms.end(), 
        [t](std::pair<TransformComponent const*,Entity const*> const& p) {
            return p.first == t;
        }));
}

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

std::optional<std::pair<TransformComponent*, Entity*>> TransformManager::PickEntity(
    double clickX, double clickY, int windowWidth, int windowHeight,
    float fovy, float aspectRatio, float zNear,
    TransformComponent const& cameraTransform) {
    // First we find the world-space position of the top-left corner of the near clipping plane.
    Vec3 nearPlaneCenter = cameraTransform.GetPos() - cameraTransform.GetZAxis() * zNear;
    float nearPlaneHalfHeight = zNear * tan(0.5f * fovy);
    float nearPlaneHalfWidth = nearPlaneHalfHeight * aspectRatio;
    Vec3 nearPlaneTopLeft = nearPlaneCenter;
    nearPlaneTopLeft -= nearPlaneHalfWidth * cameraTransform.GetXAxis();
    nearPlaneTopLeft += nearPlaneHalfHeight * cameraTransform.GetYAxis();

    // Now map clicked point from [0,windowWidth] -> [0,1]
    float xFactor = clickX / windowWidth;
    float yFactor = clickY / windowHeight;

    Vec3 clickedPointOnNearPlane = nearPlaneTopLeft;
    clickedPointOnNearPlane += (xFactor * 2 * nearPlaneHalfWidth) * cameraTransform.GetXAxis();
    clickedPointOnNearPlane -= (yFactor * 2 * nearPlaneHalfHeight) * cameraTransform.GetYAxis();

    std::optional<float> closestPickDist;
    std::pair<TransformComponent*, Entity*>* closestPickItem = nullptr;
    float pickSphereRad = 1.f;
    Vec3 rayDir = (clickedPointOnNearPlane - cameraTransform.GetPos()).GetNormalized();
    // Start ray forward of the camera a bit so we don't just pick the camera
    Vec3 rayStart = cameraTransform.GetPos() + rayDir * (pickSphereRad + 0.1f);
    for (auto& item : _transforms) {
        TransformComponent* t = item.first;
        std::optional<float> hitDist = sphereRayCast(rayStart, rayDir, t->GetPos(), pickSphereRad);
        if (hitDist.has_value()) {
            if (!closestPickDist.has_value() || *hitDist < *closestPickDist) {
                *closestPickDist = *hitDist;
                closestPickItem = &item;
            }
        }
    }

    if (closestPickItem == nullptr) {
        return std::nullopt;
    }

    return *closestPickItem;
}