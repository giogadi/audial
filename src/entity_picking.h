#pragma once

#include <memory>

class Entity;
class EntityManager;
class TransformComponent;

std::weak_ptr<Entity> PickEntity(
    EntityManager& entities, double clickX, double clickY, int windowWidth, int windowHeight,
    float fovy, float aspectRatio, float zNear, TransformComponent const& cameraTransform);